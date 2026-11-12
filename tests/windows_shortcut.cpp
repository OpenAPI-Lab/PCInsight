#include "../src/windows_shortcut.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace pcinsight;
using namespace pcinsight::detail;
using B=std::vector<std::uint8_t>;
void put(B& b,std::size_t p,std::uint32_t n){for(unsigned k=0;k<4;++k)b.at(p+k)=static_cast<std::uint8_t>(n>>(8*k));}
void word(B& b,std::uint16_t n){b.push_back(n&255);b.push_back(n>>8);}
void ascii(B& b,const std::string& s){b.insert(b.end(),s.begin(),s.end());b.push_back(0);}
void unicode(B& b,const std::u16string& s){for(auto c:s)word(b,c);word(b,0);}
void counted(B& b,const std::u16string& s){word(b,static_cast<std::uint16_t>(s.size()));for(auto c:s)word(b,c);}
B fixture(){
    B b(76);put(b,0,76);const std::uint8_t clsid[]{1,0x14,2,0,0,0,0,0,0xc0,0,0,0,0,0,0,0x46};std::copy(std::begin(clsid),std::end(clsid),b.begin()+4);put(b,20,0xb2);
    B info(36);put(info,4,36);put(info,8,1);put(info,12,36);
    info.resize(53);put(info,36,17);put(info,48,16); // Bounded VolumeID with empty ANSI label.
    put(info,16,static_cast<std::uint32_t>(info.size()));ascii(info,"C:\\App.exe");
    put(info,24,static_cast<std::uint32_t>(info.size()));ascii(info,"");
    put(info,28,static_cast<std::uint32_t>(info.size()));unicode(info,u"C:\\App\u00e9.exe");
    put(info,32,static_cast<std::uint32_t>(info.size()));unicode(info,u"");put(info,0,static_cast<std::uint32_t>(info.size()));
    b.insert(b.end(),info.begin(),info.end());counted(b,u"C:\\Work");counted(b,u"--literal \"x y\" & NEVER");b.resize(b.size()+4);return b;
}
StartupEntry row(){StartupEntry r;r.id="winfolder:user:fixture";r.name="App.LNK";r.source="fixture.lnk";return r;}
int main(){
    const auto b=fixture();auto parsed=parse_windows_shortcut(b);assert(parsed.value && parsed.value->stored_local_target.value=="C:\\App\xc3\xa9.exe" && parsed.value->arguments.value=="--literal \"x y\" & NEVER" && parsed.value->working_directory.value=="C:\\Work");
    for(std::size_t n=0;n<b.size();++n)assert(!parse_windows_shortcut(std::span(b).first(n)).value);
    auto bad=b;bad[4]=0;assert(!parse_windows_shortcut(bad).value);
    bad=b;put(bad,76,0xffffffffU);assert(!parse_windows_shortcut(bad).value);
    bad=b;put(bad,80,30);assert(!parse_windows_shortcut(bad).value);
    bad=b;put(bad,76+28,0xffffffffU);assert(!parse_windows_shortcut(bad).value);
    bad=b;put(bad,76+28,0);assert(!parse_windows_shortcut(bad).value);
    bad=b;put(bad,76+12,0);assert(!parse_windows_shortcut(bad).value);
    bad=b;bad.push_back(0);assert(!parse_windows_shortcut(bad).value);
    bad=b;put(bad,bad.size()-4,7);assert(!parse_windows_shortcut(bad).value);
    bad=b;put(bad,20,0x1b2);parsed=parse_windows_shortcut(bad);assert(parsed.value && !parsed.value->stored_local_target.value && parsed.value->arguments.value);
    bad=b;put(bad,76+8,2);parsed=parse_windows_shortcut(bad);assert(parsed.value && !parsed.value->stored_local_target.value); // Network resolution not implemented.
    // Minimal Unicode StringData, including supplementary Unicode and actual empty arguments.
    B minimal(b.begin(),b.begin()+76);put(minimal,20,0xa0);counted(minimal,u"\U0001f600");minimal.resize(minimal.size()+4);assert(parse_windows_shortcut(minimal).value->arguments.value=="\xf0\x9f\x98\x80");
    bad=minimal;bad[78]=0;bad[79]=0;assert(!parse_windows_shortcut(bad).value);
    bad=minimal;bad[80]=0;bad[81]=0;assert(!parse_windows_shortcut(bad).value);
    B empty(b.begin(),b.begin()+76);put(empty,20,128);empty.resize(80);assert(parse_windows_shortcut(empty).value->arguments.value=="");
    B legacy(empty.begin(),empty.begin()+76);put(legacy,20,32);word(legacy,1);legacy.push_back(0xe9);legacy.resize(legacy.size()+4);assert(!parse_windows_shortcut(legacy).value);
    legacy[78]='x';assert(parse_windows_shortcut(legacy).value->arguments.value=="x");
    bad.assign(1024*1024+1,0);assert(!parse_windows_shortcut(bad).value);
    unsigned reads=0;ShortcutRead read=[&](const std::string& path)->Metric<B>{assert(path=="fixture.lnk");++reads;return {b,Support::available,"fixture"};};
    std::vector<StartupEntry> rows{row()};fill_windows_startup_shortcuts(rows,read);assert(reads==1 && rows[0].windows_shortcut.value && rows[0].command.empty() && rows[0].status==StartupStatus::unknown);
    rows[0].id="winrun:fake";fill_windows_startup_shortcuts(rows,read);assert(reads==1 && !rows[0].windows_shortcut.value);
    rows={row()};rows[0].name="App.txt";fill_windows_startup_shortcuts(rows,read);assert(reads==1);
    rows.assign(129,row());reads=0;fill_windows_startup_shortcuts(rows,read);assert(reads==128 && rows[127].windows_shortcut.value && !rows[128].windows_shortcut.value);
    rows.assign(17,row());reads=0;fill_windows_startup_shortcuts(rows,[&](const std::string&)->Metric<B>{++reads;return {B(1024*1024),Support::available,"malformed full-size fixture"};});assert(reads==16 && !rows.back().windows_shortcut.value);
    rows={row()};fill_windows_startup_shortcuts(rows,[](const std::string&)->Metric<B>{return {{},Support::permission_required,"denied"};});assert(rows[0].windows_shortcut.support==Support::permission_required && !rows[0].windows_shortcut.value);
    assert(!read_windows_shortcut_file("").value);
#ifdef _WIN32
    // Test-generated .lnk bytes only, in TMP/work. No real Startup writes or target access.
    const auto dir=std::filesystem::temp_directory_path()/std::filesystem::path("pcinsight-lnk-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    assert(std::filesystem::create_directory(dir));struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code ec;std::filesystem::remove(p/"fixture.lnk",ec);std::filesystem::remove(p,ec);}} cleanup{dir};
    const auto file=dir/"fixture.lnk";{std::ofstream stream(file,std::ios::binary);stream.write(reinterpret_cast<const char*>(b.data()),static_cast<std::streamsize>(b.size()));assert(stream.good());}
    const auto u8path=file.u8string();const auto native=read_windows_shortcut_file(std::string(reinterpret_cast<const char*>(u8path.data()),u8path.size()));assert(native.value && *native.value==b && parse_windows_shortcut(*native.value).value);
    assert(!read_windows_shortcut_file(dir.string()).value);
    Monitor monitor;SampleOptions options;options.include_startup_entries=true;options.include_windows_startup_shortcut_metadata=true;options.include_processes=false;options.include_disks=false;options.include_networks=false;options.include_gpus=false;
    const auto snapshot=monitor.sample(options);for(const auto& r:snapshot.startup_entries)assert(r.command.empty() || !r.id.starts_with("winfolder:"));
    options.include_windows_startup_shortcut_metadata=false;for(const auto& r:monitor.sample(options).startup_entries)assert(!r.windows_shortcut.value);
#endif
    std::cout<<"Stored .lnk metadata parser, budgets and reader tests passed\n";
}
