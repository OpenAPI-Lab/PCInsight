#include "../src/windows_shortcut.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace pcinsight;
using namespace pcinsight::detail;
using B=std::vector<std::uint8_t>;
B bytes(const std::string& s){return {s.begin(),s.end()};}
B utf16(const std::u16string& s){B b{0xff,0xfe};for(auto c:s){b.push_back(c&255);b.push_back(c>>8);}return b;}
auto parse(const std::string& s){return parse_windows_internet_shortcut(bytes(s));}
StartupEntry row(){StartupEntry r;r.id="winfolder:user:fixture";r.name="Site.UrL";r.source="fixture.url";return r;}
int main(){
    const std::string source=";comment\r\n[Other]\r\nURL=ignored:value\r\n [InternetShortcut] \r\n URL = https://example.invalid/a?q=1&b=2#fragment \r\nIconFile=\\\\never-contact\\share\\icon.ico\r\n";
    auto result=parse(source);assert(result.value && result.value->stored_url.value=="https://example.invalid/a?q=1&b=2#fragment" && result.value->stored_url.support==Support::available && !result.value->stored_local_target.value && !result.value->arguments.value && !result.value->working_directory.value);
    assert(parse("[iNtErNeTsHoRtCuT]\nuRl=custom-app:action").value->stored_url.value=="custom-app:action");
    assert(parse("[InternetShortcut]\nURL=file:///C:/Test%20Folder/file.txt").value);
    auto wide=utf16(u"[InternetShortcut]\r\nURL=https://example.invalid/\u00e9/\U0001f600\r\n");result=parse_windows_internet_shortcut(wide);assert(result.value->stored_url.value=="https://example.invalid/\xc3\xa9/\xf0\x9f\x98\x80");
    const std::string bom="\xef\xbb\xbf";assert(parse(bom+"[InternetShortcut]\nURL=https://example.invalid/\xc3\xa9").value);
    for(const auto* s:{"", "[InternetShortcut]\n", "[Other]\nURL=https://example.invalid", "[InternetShortcut]\nURL=", "[InternetShortcut]\nURL=relative/file", "[InternetShortcut]\nURL=https:", "[InternetShortcut]\nURL=1scheme:test", "[InternetShortcut]\nURL=a_b:test", "[InternetShortcut]\nURL=https://example.invalid/a b", "[InternetShortcut]\nURL=\"https://example.invalid\"", "[InternetShortcut]\nURL=https://a\nURL=https://b", "[InternetShortcut]\nURL=https://a\nuRl=https://a", "[InternetShortcut]\nURL=https://a\n[internetshortcut]\nURL=https://a", "[InternetShortcut\nURL=https://a", "[InternetShortcut]\nmalformed-line\nURL=https://a"})assert(!parse(s).value);
    assert(!parse("[InternetShortcut]\nURL=https://example.invalid/\xe9").value); // No codepage guess.
    for(const auto* invalid:{"\xc0\xaf", "\xed\xa0\x80", "\xf4\x90\x80\x80", "\xf0\x9f", "\x80", "\xe2\x28\xa1"})assert(!parse(bom+"[InternetShortcut]\nURL=https://example.invalid/"+invalid).value);
    assert(!parse(std::string("[InternetShortcut]\nURL=https://a")+'\0'+"suffix").value);
    auto bad=wide;bad.pop_back();assert(!parse_windows_internet_shortcut(bad).value);
    bad={0xfe,0xff,0,65};assert(!parse_windows_internet_shortcut(bad).value);
    bad=utf16(u"[InternetShortcut]\nURL=https://a/");bad.push_back(0);bad.push_back(0xd8);assert(!parse_windows_internet_shortcut(bad).value);
    assert(!parse(std::string(1024*1024+1,'x')).value);
    assert(!parse("[InternetShortcut]\nURL=https://a/"+std::string(32768,'x')).value);
    assert(!parse("[InternetShortcut]\nIconFile="+std::string(65536,'x')+"\nURL=https://a").value);
    assert(!parse(std::string(4096,'\n')+"[InternetShortcut]\nURL=https://a").value);
    // Every prefix ending before a complete URL value is absent, never an empty measured URL.
    const std::string minimum="[InternetShortcut]\nURL=";for(std::size_t n=0;n<=minimum.size();++n)assert(!parse(minimum.substr(0,n)).value);
    unsigned reads=0;const auto payload=bytes(source);std::vector<StartupEntry> rows{row()};
    ShortcutRead reader=[&](const std::string& path)->Metric<B>{assert(path=="fixture.url");++reads;return {payload,Support::available,"fixture"};};
    fill_windows_startup_shortcuts(rows,reader);assert(reads==1 && rows[0].windows_shortcut.value->stored_url.value && rows[0].command.empty() && rows[0].status==StartupStatus::unknown);
    rows[0].id="winrun:fake";fill_windows_startup_shortcuts(rows,reader);assert(reads==1 && !rows[0].windows_shortcut.value);
    rows.assign(129,row());reads=0;fill_windows_startup_shortcuts(rows,reader);assert(reads==128 && !rows.back().windows_shortcut.value);
    rows={row()};fill_windows_startup_shortcuts(rows,[](const std::string&)->Metric<B>{return {{},Support::permission_required,"denied"};});assert(rows[0].windows_shortcut.support==Support::permission_required);
#ifdef _WIN32
    const auto dir=std::filesystem::temp_directory_path()/std::filesystem::path("pcinsight-url-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));assert(std::filesystem::create_directory(dir));
    struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code ec;std::filesystem::remove(p/"fixture.url",ec);std::filesystem::remove(p,ec);}} cleanup{dir};
    const auto file=dir/"fixture.url";{std::ofstream out(file,std::ios::binary);out.write(reinterpret_cast<const char*>(payload.data()),static_cast<std::streamsize>(payload.size()));assert(out.good());}
    const auto path=file.u8string();const auto native=read_windows_shortcut_file(std::string(reinterpret_cast<const char*>(path.data()),path.size()));
    assert(native.value && *native.value==payload && parse_windows_internet_shortcut(*native.value).value->stored_url.value=="https://example.invalid/a?q=1&b=2#fragment");
    Monitor monitor;SampleOptions options;options.include_startup_entries=true;options.include_windows_startup_shortcut_metadata=true;options.include_processes=false;options.include_disks=false;options.include_networks=false;options.include_gpus=false;
    const auto snapshot=monitor.sample(options);for(const auto& r:snapshot.startup_entries)if(r.windows_shortcut.value && r.windows_shortcut.value->stored_url.value)assert(r.id.starts_with("winfolder:") && r.command.empty());
    options.include_windows_startup_shortcut_metadata=false;for(const auto& r:monitor.sample(options).startup_entries)assert(!r.windows_shortcut.value);
#endif
    std::cout<<"Internet Shortcut stored URL parser, encodings, limits and read-only integration passed\n";
}
