#include "linux_zram.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <cerrno>
#include <memory>
#ifdef __linux__
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
namespace pcinsight::detail {
namespace {
template<class T> Metric<T> absent(std::string note,Support support=Support::unsupported){return {{},support,std::move(note)};}
}
Metric<LinuxZramMemory> parse_linux_zram_memory(std::string_view text){
    if(text.empty()||text.size()>4096||text.find('\0')!=text.npos)return absent<LinuxZramMemory>("Invalid/oversized zram mm_stat");
    while(!text.empty()&&text.find_last_of(" \t\r\n")==text.size()-1)text.remove_suffix(1);
    if(text.find_first_of("\r\n")!=text.npos)return absent<LinuxZramMemory>("zram mm_stat must be a single row");
    std::array<std::uint64_t,3> primary{};unsigned count=0;
    while(true){
        const auto first=text.find_first_not_of(" \t\r\n");if(first==text.npos)break;text.remove_prefix(first);
        const auto end=text.find_first_of(" \t\r\n");const auto token=text.substr(0,end);
        std::uint64_t value{};const auto [ptr,error]=std::from_chars(token.data(),token.data()+token.size(),value);
        if(error!=std::errc{}||ptr!=token.data()+token.size()||count>=64)return absent<LinuxZramMemory>("Invalid zram mm_stat counter/field bound");
        if(count<3)primary[count]=value;
        ++count;
        if(end==text.npos)break;
        text.remove_prefix(end);
    }
    if(count<7)return absent<LinuxZramMemory>("Truncated zram mm_stat: requires historical seven-field prefix");
    return {LinuxZramMemory{primary[0],primary[1],primary[2]},Support::estimated,
        "zram mm_stat bytes: original data, compressed data, allocated RAM incl metadata/fragmentation; independently sampled counters, not an atomic global compression total"};
}
#ifdef __linux__
namespace {
struct Fd {int value;~Fd(){if(value>=0)::close(value);} explicit Fd(int v):value(v){} Fd(const Fd&)=delete;};
struct CloseDirectory { void operator()(DIR* directory) const {::closedir(directory);} };
Support access_support(int error){return error==EACCES||error==EPERM?Support::permission_required:Support::unsupported;}
Metric<std::string> read_at(int directory,const char* file){
    Fd fd(::openat(directory,file,O_RDONLY|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK));
    if(fd.value<0)return absent<std::string>(std::string("Cannot open zram ")+file,access_support(errno));
    struct stat st{};if(::fstat(fd.value,&st)||!S_ISREG(st.st_mode))return absent<std::string>("zram attribute is not a regular sysfs file");
    std::string text;std::array<char,512> buffer{};
    while(true){
        const auto n=::read(fd.value,buffer.data(),buffer.size());
        if(n<0){if(errno==EINTR)continue;return absent<std::string>("Cannot read zram attribute",access_support(errno));}
        if(!n)break;
        if(static_cast<std::size_t>(n)>4096-text.size())return absent<std::string>("zram attribute exceeds 4096 bytes");
        text.append(buffer.data(),static_cast<std::size_t>(n));
    }
    return {std::move(text),Support::available,"Bounded sysfs attribute read"};
}
bool dev_number(std::string_view text){
    while(!text.empty()&&(text.back()=='\n'||text.back()=='\r'||text.back()==' '||text.back()=='\t'))text.remove_suffix(1);
    const auto colon=text.find(':');if(colon==text.npos)return false;
    for(auto part:{text.substr(0,colon),text.substr(colon+1)}){
        std::uint32_t n{};const auto [end,error]=std::from_chars(part.data(),part.data()+part.size(),n);
        if(part.empty()||error!=std::errc{}||end!=part.data()+part.size())return false;
    }
    return true;
}
bool zram_name(std::string_view name){return name.size()>4&&name.size()<=32&&name.starts_with("zram")&&name.substr(4).find_first_not_of("0123456789")==name.npos;}
Metric<LinuxZramMemory> read_device(int root,const char* name){
    Fd directory(::openat(root,name,O_RDONLY|O_DIRECTORY|O_CLOEXEC));
    if(directory.value<0)return absent<LinuxZramMemory>("Cannot pin zram directory",access_support(errno));
    struct stat pinned{},after{};if(::fstat(directory.value,&pinned))return absent<LinuxZramMemory>("Cannot identify zram directory");
    const auto before=read_at(directory.value,"dev");
    if(!before.value)return absent<LinuxZramMemory>(before.note,before.support);
    if(!dev_number(*before.value))return absent<LinuxZramMemory>("Invalid zram device number");
    const auto text=read_at(directory.value,"mm_stat");
    if(!text.value)return absent<LinuxZramMemory>(text.note,text.support);
    const auto final=read_at(directory.value,"dev");
    if(!final.value)return absent<LinuxZramMemory>(final.note,final.support);
    if(*before.value!=*final.value||::fstatat(root,name,&after,0)||after.st_dev!=pinned.st_dev||after.st_ino!=pinned.st_ino)
        return absent<LinuxZramMemory>("zram directory/device identity changed during read");
    return parse_linux_zram_memory(*text.value);
}
}
#endif
Metric<std::vector<LinuxZramDevice>> read_linux_zram(const std::string& root){
#ifdef __linux__
    if(root.empty()||root.size()>65536||root.find('\0')!=root.npos)return absent<std::vector<LinuxZramDevice>>("Invalid zram inventory root");
    std::unique_ptr<DIR,CloseDirectory> directory(::opendir(root.c_str()));
    if(!directory)return absent<std::vector<LinuxZramDevice>>("Cannot enumerate zram sysfs inventory",access_support(errno));
    std::vector<LinuxZramDevice> rows;unsigned entries=0;
    while(true){
        errno=0;const auto* entry=::readdir(directory.get());
        if(!entry){if(errno)return absent<std::vector<LinuxZramDevice>>("Failed enumerating zram inventory",access_support(errno));break;}
        const std::string_view name(entry->d_name);if(name=="."||name=="..")continue;
        if(++entries>4096)return absent<std::vector<LinuxZramDevice>>("Block inventory exceeds 4096 entries; no partial zram list");
        if(!zram_name(name))continue;
        if(rows.size()>=256)return absent<std::vector<LinuxZramDevice>>("zram inventory exceeds 256 devices; no partial list");
        rows.push_back({std::string(name),read_device(::dirfd(directory.get()),entry->d_name)});
    }
    std::sort(rows.begin(),rows.end(),[](const auto& a,const auto& b){return a.id<b.id;});
    return {std::move(rows),Support::estimated,"Visible sysfs zram devices only; empty means none observed. Rows may be unreadable; hotplug/reset and counters are not one atomic snapshot. Never sum with zswap as a universal total"};
#else
    (void)root;return absent<std::vector<LinuxZramDevice>>("Linux zram source unsupported on this platform");
#endif
}
}
