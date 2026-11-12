#include "../src/linux_process_metadata.hpp"
#include "pcinsight/monitor.hpp"
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#ifdef __linux__
#include <unistd.h>
#endif
using namespace pcinsight;
using namespace pcinsight::detail;
namespace fs=std::filesystem;
std::string desktop(const std::string& exe,const std::string& description="Test application description"){return "[Desktop Entry]\nType=Application\nName=Test\nExec=\""+exe+"\" %U\nComment="+description+"\n";}
void write(const fs::path& path,const std::string& text){std::ofstream f(path,std::ios::binary);assert(f);f<<text;f.close();assert(f);}
int main(){
    const auto entry=parse_linux_desktop_description(desktop("/opt/test app"));assert(entry&&entry->program=="/opt/test app"&&entry->comment=="Test application description");
    assert(parse_linux_desktop_description("[Desktop Entry]\nType=Application\nName=Test\nExec=app %f\nComment=Hola\\smundo")->comment=="Hola mundo");
    for(const auto& bad:std::vector<std::string>{"",desktop("/opt/app")+"Hidden=true\n",desktop("/opt/app")+"DBusActivatable=true\n",desktop("/opt/app")+"Comment=Duplicate\n",desktop("/opt/app")+"[Desktop Entry]\n",desktop("/opt/app")+"Hidden=invalid\n","[Desktop Entry]\nType=Link\nName=A\nExec=app\nComment=B","[Desktop Entry]\nType=Application\nName=A\nExec=app\nComment[es]=Solo localizado","[Desktop Entry]\nType=Application\nName=A\nExec=python /script.py\nComment=B","[Desktop Entry]\nType=Application\nName=A\nExec=app %u %f\nComment=B","[Desktop Entry]\nType=Application\nName=A\nExec=app --profile test\nComment=B",desktop("$HOME/app"),desktop("/opt/a%u"),desktop("../app"),desktop("/opt/app","bad\\nline"),desktop("/opt/app",std::string(1025,'x')),std::string(65537,'x')})assert(!parse_linux_desktop_description(bad));
    const auto text=desktop("/opt/app");assert(!parse_linux_desktop_description(text+std::string(1,'\0')));
    const auto status=parse_linux_process_status("Pid:123\nUid:0 0 0 0\nName:kernel\nState:S (sleeping)\n",123);assert(status.value);
    Metric<LinuxExecutableInfo> exe{LinuxExecutableInfo{{"/opt/app",Support::estimated,"path"},{"x64",Support::available,"ELF"},{}},Support::estimated,"exe"};
    ProcessInfo p;p.name="stale";p.description="stale";
    apply_linux_process_details(p,status,{"root",Support::estimated,"NSS"},exe,{LinuxProcessDescription{"Real comment","/metadata/app.desktop"},Support::estimated,"catalogue"});
    assert(p.linux_details.real_uid.value==0&&p.username=="root"&&p.description=="Real comment"&&p.linux_details.description_source.value=="/metadata/app.desktop");assert(p.architecture=="x64"&&p.linux_details.architecture.support==Support::available);
    apply_linux_process_details(p,{{},Support::permission_required,"denied"},{},{{},Support::unsupported,"missing"},{{},Support::unsupported,"disabled"});
    assert(p.name.empty()&&p.username.empty()&&p.description.empty()&&p.architecture.empty()&&!p.linux_details.real_uid.value&&p.linux_details.name.support==Support::permission_required);
    assert(!lookup_linux_description({{},Support::unsupported,"index absent"},exe).value);
#ifdef __linux__
    const auto root=fs::temp_directory_path()/("pcinsight-description-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));assert(fs::create_directory(root));struct Cleanup{fs::path p;~Cleanup(){std::error_code ec;fs::remove_all(p,ec);}} cleanup{root};
    const auto high=root/"high"/"applications",low=root/"low"/"applications";fs::create_directories(high);fs::create_directories(low);
    const auto id=read_linux_process_identity("/proc/self");assert(id.value);const auto native=read_linux_process_executable("/proc/self",*id.value);assert(native.value&&native.value->path.value);
    const auto binary=*native.value->path.value;write(low/"app.desktop",desktop(binary));
    auto index=read_linux_description_index({high,low},"");assert(index.value&&index.value->size()==1);auto label=lookup_linux_description(index,native);assert(label.value&&label.value->text=="Test application description"&&label.value->source==(low/"app.desktop").string());
    write(low/"app.desktop",desktop(binary,"Changed longer description"));assert(!lookup_linux_description(index,native).value);
    write(high/"app.desktop","[Desktop Entry]\nHidden=true\n");index=read_linux_description_index({high,low},"");assert(index.value&&index.value->empty());assert(fs::remove(high/"app.desktop"));
    write(high/"other.desktop",desktop(binary,"Other app association"));index=read_linux_description_index({high,low},"");assert(!lookup_linux_description(index,native).value);assert(fs::remove(high/"other.desktop"));
    fs::create_directory(high/"foo");write(high/"foo-bar.desktop",desktop(binary));write(high/"foo"/"bar.desktop",desktop(binary));write(low/"foo-bar.desktop",desktop(binary));index=read_linux_description_index({high,low},"");assert(index.value&&index.value->size()==1); // only app.desktop, collision masks lower same-ID
    assert(!read_linux_description_index({fs::path("relative")},"").value);
    assert(!read_linux_description_index(std::vector<fs::path>(65,high),"").value);
    write(high/"short.desktop","[Desktop Entry]\nType=Application\nName=Test\nExec="+fs::path(binary).filename().string()+"\nComment=By caller PATH\n");
    index=read_linux_description_index({high},fs::path(binary).parent_path().string());assert(index.value&&index.value->size()==1);assert(lookup_linux_description(index,native).value->text=="By caller PATH");assert(fs::remove(high/"short.desktop"));
    struct Environment {std::string key;std::optional<std::string> old;Environment(const char* k,const std::string& v):key(k){if(const char* value=std::getenv(k))old=value;assert(::setenv(k,v.c_str(),1)==0);}~Environment(){if(old)::setenv(key.c_str(),old->c_str(),1);else ::unsetenv(key.c_str());}};
    Environment data("XDG_DATA_HOME",(root/"high").string()),dirs("XDG_DATA_DIRS",(root/"low").string());
    SampleOptions options;options.include_app_history=false;options.include_disks=false;options.include_networks=false;options.include_gpus=false;options.include_user_sessions=false;options.include_services=false;options.include_startup_entries=false;
    Monitor monitor;auto sample=monitor.sample(options);auto own_row=[&](const SystemSnapshot& s)->const ProcessInfo&{const auto row=std::find_if(s.processes.begin(),s.processes.end(),[&](const auto& process){return process.pid==id.value->pid;});assert(row!=s.processes.end());return *row;};
    assert(own_row(sample).description=="Changed longer description"&&own_row(sample).linux_details.description.support==Support::estimated&&own_row(sample).linux_details.real_uid.value==static_cast<std::uint32_t>(::getuid()));
    options.include_linux_process_descriptions=false;sample=monitor.sample(options);assert(own_row(sample).description.empty()&&!own_row(sample).linux_details.description.value&&own_row(sample).linux_details.architecture.value);
    std::cout<<"Native XDG/private catalogue: own PID="<<id.value->pid<<" positive Comment/source, precedence/hidden/collision/ambiguity/source-change/PATH/opt-out passed; no installed application modified\n";
#else
    assert(!read_linux_description_index({},"").value);assert(!read_native_linux_description_index().value);
#endif
    std::cout<<"Desktop metadata subset, absence/permissions, availability report, stale legacy clearing passed\n";
}
