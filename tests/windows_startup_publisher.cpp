#include "../src/windows_startup_publisher.hpp"
#include <cassert>
#include <iostream>
#include <stdexcept>
using namespace pcinsight;
using namespace pcinsight::detail;
StartupEntry run(std::string command="\"C:\\Program Files\\App\\app.exe\" --literal & NEVER"){
    StartupEntry r;r.id="winrun:fixture";r.command=std::move(command);return r;
}
int main(){
    unsigned calls=0;std::string selected;
    StartupCompanyRead reader=[&](const std::string& path)->Metric<std::string>{++calls;selected=path;return {std::string("Example Company"),Support::available,"self-reported fixture"};};
    std::vector<StartupEntry> rows{run(),run()};fill_windows_startup_publishers(rows,reader);
    assert(calls==1 && selected=="C:\\Program Files\\App\\app.exe" && rows[0].publisher=="Example Company" && rows[0].publisher_label.support==Support::estimated && rows[0].publisher_source_executable.value==selected && rows[0].publisher_label.note.find("NOT verified signer")!=std::string::npos && rows[0].status==StartupStatus::unknown && rows[0].impact==StartupImpact::not_measured);
    rows={run("C:\\App.EXE --args")};fill_windows_startup_publishers(rows,reader);assert(selected=="C:\\App.EXE" && rows[0].publisher_label.value);
    for(const auto* command:{"", "app.exe", "C:app.exe", "\\\\server\\share\\app.exe", "%ProgramFiles%\\App\\app.exe", "C:\\Program Files\\App\\app.exe", "\"C:\\App.exe", "\"C:\\App.exe\"suffix", "cmd /c App.exe", "C:\\run.cmd", "C:\\a.dll", "C:/App.exe", "\"C:\\%ENV%\\App.exe\"", "C:\\App.exe:stream.exe", "C:\\*.exe", "C:\\Bad?.exe", "C:\\Bad|x.exe"}){
        const auto before=calls;rows={run(command)};fill_windows_startup_publishers(rows,reader);assert(calls==before && rows[0].publisher.empty() && !rows[0].publisher_label.value);
    }
    StartupEntry direct;direct.id="winfolder:user:fixture";direct.name="App.exe";direct.source="C:\\Folder\\App.exe";
    rows={direct};fill_windows_startup_publishers(rows,reader);assert(selected==direct.source && rows[0].publisher_label.value && rows[0].command.empty());
    auto link=direct;link.name="App.lnk";link.source="C:\\Folder\\App.lnk";
    const auto before=calls;rows={link};fill_windows_startup_publishers(rows,reader);assert(calls==before && !rows[0].publisher_label.value);
    WindowsShortcutInfo metadata;metadata.stored_local_target={std::string("C:\\Target\\app.exe"),Support::available,"stored not resolved"};link.windows_shortcut={metadata,Support::available,"fixture"};
    rows={link};fill_windows_startup_publishers(rows,reader);assert(selected=="C:\\Target\\app.exe" && rows[0].publisher_label.value);
    link.windows_shortcut.support=Support::estimated;rows={link};fill_windows_startup_publishers(rows,reader);assert(!rows[0].publisher_label.value);
    link.windows_shortcut.support=Support::available;link.windows_shortcut.value->stored_local_target.support=Support::unsupported;rows={link};fill_windows_startup_publishers(rows,reader);assert(!rows[0].publisher_label.value);
    direct.name="App.url";direct.windows_shortcut={metadata,Support::available,"injected misleading local field"};rows={direct};fill_windows_startup_publishers(rows,reader);assert(!rows[0].publisher_label.value); // Never looks up a URL handler.
    for(auto support:{Support::permission_required,Support::unsupported}){
        rows={run()};fill_windows_startup_publishers(rows,[&](const std::string&)->Metric<std::string>{return {{},support,"fixture unavailable"};});assert(rows[0].publisher.empty() && !rows[0].publisher_label.value && rows[0].publisher_label.support==support);
    }
    for(const auto& bad:std::vector<std::string>{"",std::string("a\0b",3),std::string(16385,'a')}){
        rows={run()};fill_windows_startup_publishers(rows,[&](const std::string&)->Metric<std::string>{return {bad,Support::available,"bad"};});assert(!rows[0].publisher_label.value);
    }
    rows.clear();for(unsigned k=0;k<129;++k)rows.push_back(run("C:\\App"+std::to_string(k)+".exe"));calls=0;fill_windows_startup_publishers(rows,reader);assert(calls==128 && rows[127].publisher_label.value && !rows[128].publisher_label.value);
    rows.assign(600,run());calls=0;fill_windows_startup_publishers(rows,[&](const std::string&)->Metric<std::string>{++calls;return {std::string(16384,'x'),Support::estimated,"large"};});assert(calls==1 && rows[0].publisher_label.value && !rows.back().publisher_label.value); // Output accounting includes legacy mirrored label.
    rows={run()};fill_windows_startup_publishers(rows,StartupCompanyRead{});assert(!rows[0].publisher_label.value);
    StartupEntry unrelated;unrelated.id="linux-entry";unrelated.publisher="keep";rows={unrelated};fill_windows_startup_publishers(rows,reader);assert(rows[0].publisher=="keep");
    bool failed=false;rows.assign(50001,run());try{fill_windows_startup_publishers(rows,reader);}catch(const std::length_error&){failed=true;}assert(failed);
    assert(!read_windows_startup_company("").value);
    std::cout<<"Startup self-reported publisher: path selection, confidence, cache, bounds and failures passed\n";
}
