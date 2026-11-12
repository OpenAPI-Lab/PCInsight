#include "linux_swap_disks.hpp"
#include "linux_formatted_capacity.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <fstream>
#include <filesystem>
#include <map>
#include <tuple>
#include <limits>
#ifdef __linux__
#include <cerrno>
#include <fcntl.h>
#include <linux/magic.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/vfs.h>
#include <unistd.h>
#endif
namespace pcinsight::detail {
namespace {
template<class T> bool known(const Metric<T>& m){return m.value&&(m.support==Support::available||m.support==Support::estimated);}
bool valid_disk(const std::string& id){return !id.empty()&&id.size()<=4096&&id.find_first_of("/\\\0",0,3)==id.npos&&id!="."&&id!="..";}
template<class T> bool number(std::string_view s,T& n){if(s.empty())return false;const auto [end,error]=std::from_chars(s.data(),s.data()+s.size(),n);return error==std::errc{}&&end==s.data()+s.size();}
std::vector<std::string_view> fields(std::string_view line){
    std::vector<std::string_view> result;
    while(!line.empty()){
        const auto first=line.find_first_not_of(" \t\r");if(first==line.npos)break;line.remove_prefix(first);
        const auto end=line.find_first_of(" \t\r");result.push_back(line.substr(0,end));
        if(result.size()>5||end==line.npos)break;
        line.remove_prefix(end);
    }
    return result;
}
std::optional<std::string> decode(std::string_view text){
    if(text.empty()||text.front()!='/'||text.size()>16384||text.find('\0')!=text.npos)return {};
    std::string path;
    for(std::size_t i=0;i<text.size();++i){
        if(text[i]!='\\'){path+=text[i];continue;}
        const auto s=text.substr(i,4);
        if(s=="\\040")path+=' ';else if(s=="\\011")path+='\t';else if(s=="\\012")path+='\n';else if(s=="\\134")path+='\\';else return {};
        i+=3;
    }
    return path;
}
auto configuration(const std::vector<LinuxSwapEntry>& entries){
    std::set<std::tuple<std::string,bool,std::uint64_t,std::int32_t>> keys;
    for(const auto& e:entries)keys.emplace(e.path,e.file,e.size_kib,e.priority);
    return keys;
}
}
Metric<std::vector<LinuxSwapEntry>> parse_linux_swaps(std::string_view text){
    auto fail=[](std::string note)->Metric<std::vector<LinuxSwapEntry>>{return {{},Support::unsupported,std::move(note)};};
    if(text.size()>16*1024*1024)return fail("Swap inventory exceeds 16 MiB");
    const auto end_header=text.find('\n');const auto header=fields(text.substr(0,end_header));
    if((end_header==text.npos?text.size():end_header)>65536)return fail("Swap header exceeds 64 KiB");
    if(header!=std::vector<std::string_view>{"Filename","Type","Size","Used","Priority"})return fail("Invalid/missing swap inventory header");
    if(end_header==text.npos)text={};else text.remove_prefix(end_header+1);
    std::vector<LinuxSwapEntry> entries;std::set<std::string> seen;
    while(!text.empty()){
        const auto newline=text.find('\n');const auto line=text.substr(0,newline);
        if(line.size()>65536)return fail("Swap row exceeds 64 KiB");
        if(!line.empty()){
            const auto tokens=fields(line);if(tokens.size()!=5)return fail("Invalid swap row fields");
            const auto path=decode(tokens[0]);LinuxSwapEntry entry;
            if(!path||(tokens[1]!="file"&&tokens[1]!="partition")||!number(tokens[2],entry.size_kib)||!number(tokens[3],entry.used_kib)||entry.used_kib>entry.size_kib||!number(tokens[4],entry.priority))return fail("Invalid swap row value");
            if(entries.size()>=100000||!seen.insert(*path).second)return fail("Too many/duplicate swap paths");
            entry.path=*path;entry.file=tokens[1]=="file";entries.push_back(std::move(entry));
        }
        if(newline==text.npos)break;
        text.remove_prefix(newline+1);
    }
    return {std::move(entries),Support::available,"Parsed active swap inventory"};
}
Metric<LinuxSwapDiskReport> sample_linux_swap_disks(const LinuxSwapApi& api){
    auto fail=[](std::string note)->Metric<LinuxSwapDiskReport>{return {{},Support::unsupported,std::move(note)};};
    if(!api.inventory||!api.disk)return fail("Incomplete swap provider");
    const auto before=api.inventory();if(!known(before))return {{},before.support==Support::permission_required?Support::permission_required:Support::unsupported,before.note};
    const auto parsed=parse_linux_swaps(*before.value);if(!parsed.value)return fail(parsed.note);
    LinuxSwapDiskReport report;
    for(const auto& entry:*parsed.value){
        const auto disk=api.disk(entry);
        if(known(disk)&&valid_disk(*disk.value)){
            report.disks.insert(*disk.value);
            if(api.may_exclude_others&&!api.may_exclude_others(*disk.value))report.complete=false;
        }
        else {report.complete=false;if(disk.support==Support::permission_required)report.unresolved_support=Support::permission_required;}
    }
    const auto after=api.inventory();if(!known(after))return {{},after.support==Support::permission_required?Support::permission_required:Support::unsupported,after.note};
    const auto repeated=parse_linux_swaps(*after.value);
    if(!repeated.value||configuration(*parsed.value)!=configuration(*repeated.value))return fail("Swap configuration changed or became invalid; resample");
    return {std::move(report),Support::estimated,"Current pathname-to-block association of active swap in two matching configurations; not physical-disk attribution or pinned kernel swap inode identity"};
}
void apply_linux_swap_disk_flags(std::vector<DiskInfo>& disks,const Metric<LinuxSwapDiskReport>& sample){
    std::map<std::string,std::size_t> counts;
    for(const auto& disk:disks)++counts[disk.id];
    bool complete=known(sample)&&sample.value->complete;
    if(known(sample))for(const auto& id:sample.value->disks){
        const auto found=counts.find(id);
        if(!valid_disk(id)||found==counts.end()||found->second!=1)complete=false;
    }
    for(auto& disk:disks){
        if(!known(sample)){disk.contains_page_file={{},sample.support==Support::permission_required?Support::permission_required:Support::unsupported,sample.note.empty()?"Active swap inventory unavailable":sample.note};continue;}
        const auto count=counts.at(disk.id);
        if(!valid_disk(disk.id)||count!=1){disk.contains_page_file={{},Support::unsupported,"Invalid/ambiguous disk inventory identity"};continue;}
        if(sample.value->disks.contains(disk.id))disk.contains_page_file={true,Support::estimated,sample.note};
        else if(complete)disk.contains_page_file={false,Support::estimated,"No listed active swap mapped directly to this block device in two observations; not a search for inactive swap files/partitions"};
        else disk.contains_page_file={{},sample.value->unresolved_support==Support::permission_required?Support::permission_required:Support::unsupported,"Some active swap paths/block mappings unresolved; cannot establish absence on this disk"};
    }
}
#ifdef __linux__
Metric<std::string> read_linux_swap_path_disk_with_resolver(const LinuxSwapEntry& entry,
    const std::function<std::optional<std::string>(const std::string&)>& resolve){
    auto fail=[](std::string note)->Metric<std::string>{return {{},Support::unsupported,std::move(note)};};
    if(!resolve||entry.path.empty()||entry.path.front()!='/'||entry.path.size()>16384||entry.path.find('\0')!=entry.path.npos||entry.used_kib>entry.size_kib)return fail("Invalid swap reference path/counters/resolver");
    const int fd=open(entry.path.c_str(),O_PATH|O_CLOEXEC);
    if(fd<0)return {{},errno==EACCES||errno==EPERM?Support::permission_required:Support::unsupported,"Active swap pathname not accessible in caller view"};
    struct Owned{int fd;~Owned(){close(fd);}} owned{fd};
    struct stat info{};if(fstat(fd,&info)!=0)return fail("Cannot identify swap path reference");
    if(entry.file?!S_ISREG(info.st_mode):!S_ISBLK(info.st_mode))return fail("Swap path type differs from kernel listing");
    if(entry.file){
        if(info.st_size<0||entry.size_kib>std::numeric_limits<std::uint64_t>::max()/1024||
           entry.size_kib*1024>static_cast<std::uint64_t>(info.st_size))return fail("Swap pathname file is smaller than listed usable size or size overflows bytes");
        struct statfs fs{};if(fstatfs(fd,&fs)!=0)return fail("Cannot identify swap file filesystem");
        if(fs.f_type!=EXT2_SUPER_MAGIC)return fail("Swap file filesystem topology requires a dedicated provider (initial file support: ext2/3/4)");
    }
    const auto device=entry.file?info.st_dev:info.st_rdev;
    const auto disk=resolve(std::to_string(major(device))+":"+std::to_string(minor(device)));
    if(!disk||!valid_disk(*disk))return fail("Swap reference has no valid visible direct block-device mapping");
    struct stat after{},named{};
    if(fstat(fd,&after)||fstatat(AT_FDCWD,entry.path.c_str(),&named,0))return fail("Swap pathname disappeared or cannot be revalidated after mapping");
    const auto same=[](const struct stat& a,const struct stat& b){
        return a.st_dev==b.st_dev&&a.st_ino==b.st_ino&&a.st_mode==b.st_mode&&a.st_rdev==b.st_rdev&&
            a.st_size==b.st_size&&a.st_nlink==b.st_nlink&&a.st_mtim.tv_sec==b.st_mtim.tv_sec&&a.st_mtim.tv_nsec==b.st_mtim.tv_nsec&&
            a.st_ctim.tv_sec==b.st_ctim.tv_sec&&a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
    };
    if(!same(info,after)||!same(after,named))return fail("Swap pathname reference changed identity/metadata during mapping");
    return {*disk,Support::estimated,"Current pathname/metadata endpoints match and file size covers listed usable swap; direct block mapping only, not kernel active swap inode identity"};
}
Metric<std::string> read_linux_swap_path_disk(const LinuxSwapEntry& entry){
    return read_linux_swap_path_disk_with_resolver(entry,resolve_linux_block_device);
}
Metric<LinuxSwapDiskReport> read_native_linux_swap_disks(){
    LinuxSwapApi api;api.disk=read_linux_swap_path_disk;
    api.may_exclude_others=[](const std::string& disk){
        std::error_code error;
        const auto base=std::filesystem::path("/sys/block")/disk;
        const auto path=std::filesystem::canonical(base,error);if(error)return false;
        for(const auto& part:path)if(part=="virtual")return false;
        std::filesystem::directory_iterator slaves(base/"slaves",error);
        return !error&&slaves==std::filesystem::directory_iterator{};
    };
    api.inventory=[]()->Metric<std::string>{
        std::ifstream input("/proc/swaps",std::ios::binary);if(!input)return {{},Support::unsupported,"Active swap inventory unavailable"};
        std::string text;std::array<char,4096> chunk{};
        while(input){input.read(chunk.data(),chunk.size());const auto count=static_cast<std::size_t>(input.gcount());if(count>16*1024*1024-text.size())return {{},Support::unsupported,"Swap inventory exceeds 16 MiB"};text.append(chunk.data(),count);}
        if(!input.eof())return {{},Support::unsupported,"Swap inventory read failed"};
        return {std::move(text),Support::available,"/proc/swaps"};
    };
    return sample_linux_swap_disks(api);
}
#else
Metric<std::string> read_linux_swap_path_disk_with_resolver(const LinuxSwapEntry&,const std::function<std::optional<std::string>(const std::string&)>&){return {{},Support::unsupported,"Linux swap path mapping unsupported on this platform"};}
Metric<std::string> read_linux_swap_path_disk(const LinuxSwapEntry&){return {{},Support::unsupported,"Linux swap path mapping unsupported on this platform"};}
Metric<LinuxSwapDiskReport> read_native_linux_swap_disks(){return {{},Support::unsupported,"Linux swap inventory unsupported on this platform"};}
#endif
}
