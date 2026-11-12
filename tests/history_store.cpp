#include "pcinsight/history_store.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <vector>
using namespace std::chrono;
using pcinsight::HistoryStore;
void check(bool ok){if(!ok)throw std::runtime_error("history regression");}
std::string bytes(const std::filesystem::path& p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
void write(const std::filesystem::path& p,const std::string& s){std::ofstream f(p,std::ios::binary);f.write(s.data(),s.size());}
int main(){
    auto dir=std::filesystem::temp_directory_path()/("pcinsight-history-"+std::to_string(steady_clock::now().time_since_epoch().count()));
    check(std::filesystem::create_directory(dir));
    auto path=dir/"history.bin", corrupt=dir/"corrupt.bin";
    pcinsight::LegacyAppHistoryEntry a;
    a.id="/usr/bin/demo";a.name="demo";a.cpu_time=seconds(2);a.network_bytes=42;a.notifications=3;
    a.period_start=system_clock::time_point(seconds(10));a.period_end=system_clock::time_point(seconds(20));
    std::vector<pcinsight::LegacyAppHistoryEntry> entries{a};
    check(bool(HistoryStore::save(path,entries)));
    auto loaded=HistoryStore::load(path);check(bool(loaded)&&loaded.entries.size()==1);
    check(loaded.entries[0].cpu_time==seconds(2));
    check(loaded.format_version==2);
    // v1 fixture: same body without checksum and version set to 1.
    auto legacy=bytes(path);legacy.resize(legacy.size()-8);legacy[8]=1;
    auto legacy_path=dir/"legacy.bin";write(legacy_path,legacy);
    auto legacy_read=HistoryStore::load(legacy_path);
    check(bool(legacy_read)&&legacy_read.format_version==1&&legacy_read.entries[0].cpu_time==seconds(2));
    check(bool(HistoryStore::migrate(legacy_path)));
    check(HistoryStore::load(legacy_path).format_version==2);
    auto migrated=bytes(legacy_path);check(bool(HistoryStore::migrate(legacy_path)));
    check(bytes(legacy_path)==migrated);
    auto repeated=HistoryStore::merge(loaded.entries,entries);
    check(repeated.size()==1&&repeated[0].cpu_time==seconds(2));
    auto newer=a;newer.period_end+=seconds(5);newer.cpu_time=seconds(3);newer.network_bytes=50;
    entries={newer};
    auto merged=HistoryStore::merge(repeated,entries);check(merged.size()==1&&merged[0].cpu_time==seconds(3));
    entries={a};auto stale=HistoryStore::merge(merged,entries);check(stale[0].cpu_time==seconds(3));
    auto bad=newer;bad.cpu_time=seconds(1);entries={bad};bool threw=false;
    try{(void)HistoryStore::merge(merged,entries);}catch(const std::invalid_argument&){threw=true;}check(threw);
    auto another=a;another.period_start+=seconds(100);another.period_end+=seconds(100);
    entries={another};auto sessions=HistoryStore::merge(merged,entries);check(sessions.size()==2);
    check(bool(HistoryStore::save(path,sessions)));check(HistoryStore::load(path).entries.size()==2);
    const auto good=bytes(path);
    // A valid-looking counter mutation is caught by v2 checksum.
    auto damaged=good;damaged[20]^=1;write(corrupt,damaged);
    check(!HistoryStore::load(corrupt));check(!HistoryStore::migrate(corrupt));
    check(!HistoryStore::prune(corrupt,system_clock::time_point(seconds(100))));
    check(bytes(corrupt)==damaged);
    check(HistoryStore::retain_since(sessions,system_clock::time_point(seconds(25))).size()==2);
    check(HistoryStore::retain_since(sessions,system_clock::time_point(seconds(26))).size()==1);
    check(bool(HistoryStore::prune(legacy_path,system_clock::time_point(seconds(21)))));
    check(HistoryStore::load(legacy_path).entries.empty());
    check(bool(HistoryStore::prune(legacy_path,system_clock::time_point(seconds(21)))));
    // Every truncation must fail without exposing partial entries.
    for(std::size_t n=0;n<good.size();++n){
        write(corrupt,good.substr(0,n));auto result=HistoryStore::load(corrupt);
        check(!result&&result.entries.empty());
    }
    auto mutated=good;mutated[8]=99;write(corrupt,mutated);check(!HistoryStore::load(corrupt));
    write(corrupt,good+"junk");check(!HistoryStore::load(corrupt));
    mutated=good;
    const auto cpu_offset=16+4+a.id.size()+4+a.name.size();
    mutated[cpu_offset+7]=static_cast<char>(0xff);write(corrupt,mutated);check(!HistoryStore::load(corrupt));
    bad=a;bad.cpu_time=seconds(-1);entries={bad};
    check(!HistoryStore::save(path,entries));check(bytes(path)==good);
    bad=a;bad.period_end=bad.period_start-seconds(1);entries={bad};check(!HistoryStore::save(path,entries));
    // Unrelated backup files must never be removed.
    auto backup=path;backup+=".backup";write(backup,"keep");
    entries={a};check(bool(HistoryStore::save(path,entries)));check(bytes(backup)=="keep");
    auto occupied=dir/"occupied";check(std::filesystem::create_directory(occupied));write(occupied/"sentinel","keep");
    check(!HistoryStore::save(occupied,entries));check(bytes(occupied/"sentinel")=="keep");
    check(!HistoryStore::reset(occupied));
    for(const auto& e:std::filesystem::directory_iterator(dir))check(e.path().filename().string().find(".tmp-")==std::string::npos);
    check(bool(HistoryStore::reset(path)));check(bool(HistoryStore::reset(path)));
    std::filesystem::remove(corrupt);std::filesystem::remove(backup);std::filesystem::remove(legacy_path);
    std::filesystem::remove(occupied/"sentinel");std::filesystem::remove(occupied);std::filesystem::remove(dir);
}
