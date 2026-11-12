#include "pcinsight/startup_store.hpp"
#include "pcinsight/startup_overlay.hpp"
#include <cassert>
#include <fstream>
#include <iterator>
#include <limits>
#include <iostream>
#ifdef __linux__
#include <sys/stat.h>
#include <unistd.h>
#elif defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
using namespace pcinsight;
using namespace std::chrono_literals;
namespace fs=std::filesystem;
std::string read(const fs::path& path) {std::ifstream in(path,std::ios::binary);return {std::istreambuf_iterator<char>(in),{}};}
void write(const fs::path& path,const std::string& bytes) {std::ofstream out(path,std::ios::binary|std::ios::trunc);out.write(bytes.data(),bytes.size());assert(out.good());}
std::uint64_t hash(std::string_view bytes) {std::uint64_t n=14695981039346656037ULL;for(unsigned char c:bytes){n^=c;n*=1099511628211ULL;}return n;}
void repair(std::string& bytes) {const auto n=hash(std::string_view(bytes).substr(0,bytes.size()-8));for(unsigned i=0;i<8;++i)bytes[bytes.size()-8+i]=static_cast<char>((n>>(8*i))&255);}
StartupImpactArchive capture() {
    StartupImpactArchive archive;archive.capture_id="capture-1";archive.native_context="boot:namespace";
    archive.recorded_at=std::chrono::system_clock::time_point(123456ms);
    StartupProcessSample baseline;baseline.pid=10;baseline.linux_start_ticks=100;baseline.native_context=archive.native_context;
    baseline.sampled_at=std::chrono::steady_clock::time_point(1s);
    baseline.cumulative_cpu_time={0ns,Support::available,"zero"};baseline.cumulative_disk_bytes={0,Support::available,"zero"};
    StartupImpactObserver observer(100ms);assert(observer.register_process("app.desktop",baseline));
    auto final=baseline;final.sampled_at+=100ms;final.cumulative_cpu_time.value=20ms;assert(observer.observe_sample(final));
    auto other=baseline;other.pid=11;assert(observer.register_process("unknown.desktop",other));
    observer.close();archive.observations=observer.observations();
    archive.observations[1].sampling_gap=true;archive.observations[1].last_error=ActionError::permission_denied;
    return archive;
}
int main() {
    fs::path root;
    for(unsigned i=0;i<100;++i) {
        const auto candidate=fs::temp_directory_path()/("pcinsight-startup-store-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+"-"+std::to_string(i));
        if(fs::create_directory(candidate)){root=candidate;break;}
    }
    assert(!root.empty());
    struct Cleanup {fs::path path;~Cleanup(){std::error_code ec;fs::remove_all(path,ec);}} cleanup{root};
    const auto path=root/"capture.bin";auto archive=capture();
    assert(StartupImpactStore::load(path).error==ActionError::not_found);
    auto saved=StartupImpactStore::save(path,archive);if(!saved)std::cerr<<"legacy initial save error="<<static_cast<unsigned>(saved.error)<<" replaced="<<saved.replaced<<" note="<<saved.message<<'\n';assert(saved && saved.replaced);
    const auto original=read(path);auto loaded=StartupImpactStore::load(path);
    assert(loaded && loaded.archive->capture_id==archive.capture_id && loaded.archive->native_context==archive.native_context);
    assert(!loaded.archive->diagnostics); // Legacy v1 has UNKNOWN diagnostics, not measured zeroes.
    assert(loaded.archive->recorded_at==archive.recorded_at && loaded.archive->observations.size()==2);
    const auto& row=loaded.archive->observations[0];
    assert(row.closed && row.window_complete && !row.sampling_gap && row.last_error==ActionError::none && row.sample_count==2);
    assert(row.native_context==archive.native_context && row.measurement->cpu_time.value==20ms);
    assert(row.measurement->cpu_time.support==Support::estimated && row.measurement->disk_bytes.value==0);
    assert(row.measurement->impact.value==StartupImpact::low && row.measurement->observation_interval==100ms);
    assert(!loaded.archive->observations[1].measurement && loaded.archive->observations[1].last_error==ActionError::permission_denied);
    assert(StartupImpactStore::save(path,*loaded.archive) && read(path)==original); // deterministic overwrite, no accumulation
    SystemSnapshot snapshot;StartupEntry entry;entry.id="app.desktop";snapshot.startup_entries.push_back(entry);
    assert(apply_startup_impact_observations(snapshot,loaded.archive->observations,"different-boot").error==ActionError::stale_target);
    assert(snapshot.startup_entries[0].impact==StartupImpact::not_measured);
    // Invalid saves preserve an existing capture byte-for-byte.
    auto invalid=archive;invalid.observations[0].closed=false;
    assert(!StartupImpactStore::save(path,invalid) && read(path)==original);
    invalid=archive;invalid.observations.push_back(invalid.observations[0]);assert(!StartupImpactStore::save(path,invalid));
    invalid=archive;invalid.observations[0].measurement->cpu_time.value=-1ns;assert(!StartupImpactStore::save(path,invalid));
    invalid=archive;invalid.observations[0].measurement->impact.support=static_cast<Support>(255);assert(!StartupImpactStore::save(path,invalid));
    invalid=archive;invalid.observations[0].last_error=static_cast<ActionError>(255);assert(!StartupImpactStore::save(path,invalid));
    invalid=archive;invalid.native_context="other";assert(!StartupImpactStore::save(path,invalid));
    invalid=archive;invalid.capture_id=std::string("bad\0id",6);assert(!StartupImpactStore::save(path,invalid));
    invalid=archive;invalid.recorded_at=std::chrono::system_clock::time_point(-1ms);assert(!StartupImpactStore::save(path,invalid));
    invalid=archive;invalid.observations[0].measurement->cpu_time.note=std::string(4097,'x');assert(!StartupImpactStore::save(path,invalid));
    assert(!StartupImpactStore::save(root/"missing"/"file",archive));
    assert(!StartupImpactStore::save(root,archive));
    assert(!StartupImpactStore::load(root));assert(!StartupImpactStore::load({}));
    assert(!StartupImpactStore::save({},archive));assert(read(path)==original);
    const auto corrupt=root/"corrupt.bin";
    // Every truncation, byte corruption and appended data expose no partial rows.
    for(std::size_t n=0;n<original.size();++n){write(corrupt,original.substr(0,n));loaded=StartupImpactStore::load(corrupt);assert(!loaded && !loaded.archive);}
    for(std::size_t n=0;n<original.size();++n){auto bytes=original;bytes[n]^=1;write(corrupt,bytes);loaded=StartupImpactStore::load(corrupt);assert(!loaded && !loaded.archive);}
    auto bytes=original;bytes.push_back('x');write(corrupt,bytes);assert(!StartupImpactStore::load(corrupt));
    // Recomputed checksum does not bypass version/count/enum validation.
    bytes=original;bytes[8]=4;repair(bytes);write(corrupt,bytes);assert(!StartupImpactStore::load(corrupt));
    const std::size_t count_offset=12+4+archive.capture_id.size()+4+archive.native_context.size()+8;
    bytes=original;for(unsigned i=0;i<4;++i)bytes[count_offset+i]=static_cast<char>(255);repair(bytes);write(corrupt,bytes);assert(!StartupImpactStore::load(corrupt));
    const std::size_t flags_offset=count_offset+4+4+archive.observations[0].startup_entry_id.size()+4+8+8;
    bytes=original;bytes[flags_offset]=0;repair(bytes);write(corrupt,bytes);assert(!StartupImpactStore::load(corrupt));
    bytes=original;bytes[flags_offset+1]=static_cast<char>(255);repair(bytes);write(corrupt,bytes);assert(!StartupImpactStore::load(corrupt));
    bytes=original;bytes[flags_offset+3+8]=static_cast<char>(255);repair(bytes);write(corrupt,bytes);assert(!StartupImpactStore::load(corrupt));
    bytes=original;for(unsigned i=0;i<8;++i)bytes[flags_offset+3+8+2+i]=static_cast<char>(255);repair(bytes);write(corrupt,bytes);assert(!StartupImpactStore::load(corrupt));
    bytes=original;bytes[flags_offset]=7;repair(bytes);write(corrupt,bytes);assert(!StartupImpactStore::load(corrupt));
    bytes=original.substr(0,original.size()-8)+"junk"+std::string(8,'\0');repair(bytes);write(corrupt,bytes);assert(!StartupImpactStore::load(corrupt));
    fs::resize_file(corrupt,16*1024*1024+1);loaded=StartupImpactStore::load(corrupt);assert(!loaded && !loaded.archive);
    // Empty capture is valid; newer capture replaces rather than summing runs.
    auto empty=archive;empty.observations.clear();assert(StartupImpactStore::save(corrupt,empty));
    assert(StartupImpactStore::load(corrupt).archive->observations.empty());
    auto next=archive;next.capture_id="capture-2";next.recorded_at+=1s;assert(StartupImpactStore::save(path,next));
    assert(StartupImpactStore::load(path).archive->capture_id=="capture-2");
    // Preserve unavailable/protected counters as unknown, not zero/available.
    auto partial=archive;auto& m=*partial.observations[0].measurement;
    m.disk_bytes={std::nullopt,Support::permission_required,"protected"};m.impact={std::nullopt,Support::permission_required,"incomplete"};
    partial.observations[0].window_complete=false;partial.observations[0].sampling_gap=true;
    assert(StartupImpactStore::save(corrupt,partial));loaded=StartupImpactStore::load(corrupt);assert(loaded);
    assert(!loaded.archive->observations[0].measurement->disk_bytes.value && loaded.archive->observations[0].measurement->disk_bytes.support==Support::permission_required);
    // Version2 keeps diagnostic metadata in the SAME atomic capture file.
    const auto diagnostic_path=root/"diagnostics.bin";auto detailed=archive;
    StartupCaptureDiagnostics d;d.requested_duration=100ms;d.scan_interval=20ms;d.elapsed=150ms;d.scans=5;d.partial_scans=2;
    d.examined=8;d.ignored=1;d.unavailable=2;d.registered=2;d.rejected=3;d.poll_attempted=4;d.poll_updated=3;d.poll_failed=1;
    d.exit_disk_enabled=true;d.max_exit_disk_captures=1;d.exit_disk_attempted=1;d.exit_disk_attached=1;d.exit_disk_skipped=1;d.exit_disk_released_without_final=1;
    d.automatic_startup_origin={{},Support::unsupported,"untrusted hints; not proof"};detailed.diagnostics=d;
    assert(StartupImpactStore::save(diagnostic_path,detailed));const auto v2=read(diagnostic_path);assert(v2[8]==2&&original[8]==1);
    loaded=StartupImpactStore::load(diagnostic_path);assert(loaded&&loaded.archive->diagnostics);
    const auto& restored=*loaded.archive->diagnostics;
    assert(restored.requested_duration==100ms&&restored.scan_interval==20ms&&restored.elapsed==150ms&&restored.scans==5&&restored.partial_scans==2);
    assert(restored.examined==8&&restored.ignored==1&&restored.unavailable==2&&restored.registered==2&&restored.rejected==3);
    assert(restored.poll_attempted==4&&restored.poll_updated==3&&restored.poll_failed==1);
    assert(restored.exit_disk_enabled&&restored.max_exit_disk_captures==1&&restored.exit_disk_attempted==1&&restored.exit_disk_attached==1&&restored.exit_disk_skipped==1&&restored.exit_disk_released_without_final==1);
    assert(!restored.exit_disk_unavailable&&!restored.exit_disk_recovered&&!restored.automatic_startup_origin.value&&restored.automatic_startup_origin.note==d.automatic_startup_origin.note);
    assert(StartupImpactStore::save(diagnostic_path,*loaded.archive)&&read(diagnostic_path)==v2);
    for(unsigned mode=0;mode<15;++mode){auto bad=detailed;auto& v=*bad.diagnostics;
        switch(mode){case 0:v.partial_scans=6;break;case 1:v.registered=3;break;case 2:v.poll_failed=2;break;case 3:v.exit_disk_enabled=false;break;
        case 4:v.max_exit_disk_captures=0;break;case 5:v.exit_disk_recovered=2;break;case 6:v.exit_disk_released_without_final=0;break;
        case 7:v.requested_duration=2h;break;case 8:v.elapsed=-1ns;break;case 9:v.automatic_startup_origin.support=static_cast<Support>(255);break;
        case 10:v.automatic_startup_origin.note=std::string(4097,'x');break;case 11:v.exit_disk_unavailable=UINT64_MAX;break;case 12:v.exit_disk_attempted=UINT64_MAX;break;
        case 13:v.max_exit_disk_captures=33;break;case 14:v.exit_disk_recovered=1;break;}
        assert(!StartupImpactStore::save(diagnostic_path,bad)&&read(diagnostic_path)==v2);
    }
    for(std::size_t n=0;n<v2.size();++n){write(corrupt,v2.substr(0,n));loaded=StartupImpactStore::load(corrupt);assert(!loaded&&!loaded.archive);}
    for(std::size_t n=0;n<v2.size();++n){auto broken=v2;broken[n]^=1;write(corrupt,broken);assert(!StartupImpactStore::load(corrupt));}
    const auto extension=original.size()-8; // v2 rows retain the exact v1 shape.
    for(std::size_t n=extension;n<v2.size()-8;++n){auto broken=v2.substr(0,n)+std::string(8,'\0');repair(broken);write(corrupt,broken);assert(!StartupImpactStore::load(corrupt));}
    for(auto offset:{extension,extension+24,extension+25,extension+29+5*8,extension+29+9*8,extension+29+13*8,extension+29+16*8,extension+29+16*8+1}){
        auto broken=v2;if(offset==extension)for(unsigned n=0;n<8;++n)broken[offset+n]=static_cast<char>(255);else broken[offset]=static_cast<char>(255);repair(broken);write(corrupt,broken);assert(!StartupImpactStore::load(corrupt));
    }
    auto false_origin=detailed;false_origin.diagnostics->automatic_startup_origin={false,Support::estimated,"explicit false, not absent"};
    assert(StartupImpactStore::save(corrupt,false_origin));loaded=StartupImpactStore::load(corrupt);assert(loaded&&loaded.archive->diagnostics->automatic_startup_origin.value==false);
    auto invalid_bool=read(corrupt);invalid_bool[extension+29+16*8+2]=2;repair(invalid_bool);write(corrupt,invalid_bool);assert(!StartupImpactStore::load(corrupt));
    auto trailing=v2.substr(0,v2.size()-8)+"junk"+std::string(8,'\0');repair(trailing);write(corrupt,trailing);assert(!StartupImpactStore::load(corrupt));
    auto zero=archive;zero.observations.clear();StartupCaptureDiagnostics zero_d;zero_d.requested_duration=100ms;zero_d.scan_interval=20ms;zero.diagnostics=zero_d;
    assert(StartupImpactStore::save(corrupt,zero));loaded=StartupImpactStore::load(corrupt);assert(loaded&&loaded.archive->diagnostics&&loaded.archive->diagnostics->scans==0);
    assert(read(path)!=v2); // Independent legacy capture was not migrated implicitly.
#ifdef __linux__
    struct stat info{};assert(stat(path.c_str(),&info)==0 && (info.st_mode&0777)==0600);
    const auto symlink=root/"link";fs::create_symlink(path,symlink);
    const auto current=read(path);assert(!StartupImpactStore::load(symlink) && !StartupImpactStore::save(symlink,archive) && read(path)==current);
    const auto fifo=root/"fifo";assert(mkfifo(fifo.c_str(),0600)==0);
    assert(!StartupImpactStore::load(fifo) && !StartupImpactStore::save(fifo,archive));
    if(geteuid()!=0) {
        assert(chmod(root.c_str(),0500)==0);
        saved=StartupImpactStore::save(path,archive);
        assert(!saved && !saved.replaced && saved.error==ActionError::permission_denied && read(path)==current);
        assert(chmod(root.c_str(),0700)==0);
    }
#elif defined(_WIN32)
    const auto current=read(path);
    HANDLE held=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    assert(held!=INVALID_HANDLE_VALUE);
    saved=StartupImpactStore::save(path,archive);
    assert(!saved && !saved.replaced && read(path)==current);
    assert(CloseHandle(held));
#endif
    for(const auto& file:fs::directory_iterator(root))assert(file.path().extension()!=".tmp");
}
