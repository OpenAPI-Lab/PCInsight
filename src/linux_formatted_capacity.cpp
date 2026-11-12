#include "linux_formatted_capacity.hpp"
#include <array>
#include <charconv>
#include <fstream>
#include <filesystem>
#include <limits>
#include <vector>
#ifdef __linux__
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/sysmacros.h>
#include <unistd.h>
#endif
namespace pcinsight::detail {
namespace {
template<class T> bool number(std::string_view text,T& value){
    if(text.empty())return false;
    const auto [end,error]=std::from_chars(text.data(),text.data()+text.size(),value);
    return error==std::errc{}&&end==text.data()+text.size();
}
std::optional<std::string> device_id(std::string_view text){
    const auto colon=text.find(':');if(colon==text.npos)return {};
    std::uint32_t major{},minor{};
    if(!number(text.substr(0,colon),major)||!number(text.substr(colon+1),minor))return {};
    return std::to_string(major)+":"+std::to_string(minor);
}
std::optional<std::string> path_decode(std::string_view text){
    if(text.empty()||text.front()!='/'||text.size()>16384)return {};
    std::string path;
    for(std::size_t i=0;i<text.size();++i){
        if(text[i]=='\0')return {};
        if(text[i]!='\\'){path+=text[i];continue;}
        const auto escape=text.substr(i,4);
        if(escape=="\\040")path+=' ';
        else if(escape=="\\011")path+='\t';
        else if(escape=="\\012")path+='\n';
        else if(escape=="\\134")path+='\\';
        else return {};
        i+=3;
    }
    return path;
}
Metric<std::uint64_t> missing(std::string note){return {{},Support::unsupported,std::move(note)};}
}
bool linux_capacity_filesystem_supported(std::string_view type){
    for(auto candidate:{"ext2","ext3","ext4","vfat","msdos","exfat","ntfs","ntfs3","iso9660","udf","squashfs"})if(type==candidate)return true;
    return false;
}
std::optional<LinuxCapacityMount> parse_linux_capacity_mount(std::string_view line){
    if(line.empty()||line.size()>65536||line.find('\0')!=line.npos)return {};
    std::vector<std::string_view> fields;
    while(!line.empty()){
        const auto begin=line.find_first_not_of(" \t\r");if(begin==line.npos)break;line.remove_prefix(begin);
        const auto end=line.find_first_of(" \t\r");fields.push_back(line.substr(0,end));
        if(fields.size()>1024)return {};
        if(end==line.npos)break;
        line.remove_prefix(end);
    }
    if(fields.size()<10)return {};
    std::uint64_t id{},parent{};
    const auto device=device_id(fields[2]);const auto root=path_decode(fields[3]),path=path_decode(fields[4]);
    if(!number(fields[0],id)||!number(fields[1],parent)||!device||!root||!path)return {};
    std::size_t separator=6;while(separator<fields.size()&&fields[separator]!="-")++separator;
    if(separator+4!=fields.size())return {};
    return LinuxCapacityMount{*device,*path,std::string(fields[separator+1])};
}
Metric<std::uint64_t> linux_filesystem_capacity(std::uint64_t blocks,std::uint64_t fragment){
    if(!fragment)return missing("Filesystem fragment size is zero");
    if(blocks>std::numeric_limits<std::uint64_t>::max()/fragment)return missing("Filesystem capacity exceeds uint64 range");
    return {blocks*fragment,Support::available,"Filesystem f_blocks * f_frsize, not free space or raw disk capacity"};
}
Metric<LinuxFormattedMap> aggregate_linux_formatted_capacity(std::string_view text,const LinuxFormattedApi& api){
    auto fail=[](std::string note)->Metric<LinuxFormattedMap>{return {{},Support::unsupported,std::move(note)};};
    if(!api.disk||!api.capacity)return fail("Incomplete filesystem capacity provider");
    if(text.size()>16*1024*1024)return fail("Mount inventory exceeds 16 MiB");
    // Validate all input before performing filesystem queries; no partial parse results.
    std::vector<LinuxCapacityMount> mounts;
    while(!text.empty()){
        const auto newline=text.find('\n');const auto line=text.substr(0,newline);
        if(!line.empty()){
            const auto parsed=parse_linux_capacity_mount(line);if(!parsed)return fail("Invalid mountinfo row");
            if(mounts.size()>=100000)return fail("Mount inventory exceeds row limit");
            mounts.push_back(*parsed);
        }
        if(newline==text.npos)break;
        text.remove_prefix(newline+1);
    }
    struct Observed {Metric<std::uint64_t> result;bool conflict{};std::string filesystem;};
    std::map<std::string,std::optional<std::string>> disks;
    std::map<std::string,std::map<std::string,Observed>> observations;
    for(const auto& mount:mounts){
        auto it=disks.find(mount.device);
        if(it==disks.end())it=disks.emplace(mount.device,api.disk(mount.device)).first;
        if(!it->second)continue;
        const auto& disk=*it->second;
        if(disk.empty()||disk.size()>4096||disk.find_first_of("/\\\0",0,3)!=disk.npos)return fail("Invalid block mapping identity");
        auto& observed=observations[disk][mount.device];
        if(!observed.filesystem.empty()&&observed.filesystem!=mount.filesystem)observed.conflict=true;
        observed.filesystem=mount.filesystem;
        const auto value=linux_capacity_filesystem_supported(mount.filesystem)?api.capacity(mount):missing("Filesystem topology requires a dedicated provider");
        if(value.value&&(value.support==Support::available||value.support==Support::estimated)){
            if(observed.result.value&&observed.result.value!=value.value)observed.conflict=true;
            observed.result=value;
        }else if(!observed.result.value){
            observed.result=value;observed.result.value.reset();
            if(observed.result.support!=Support::permission_required)observed.result.support=Support::unsupported;
        }
    }
    LinuxFormattedMap output;
    for(const auto& [disk,devices]:observations){
        std::uint64_t total=0;Metric<std::uint64_t> result;
        for(const auto& [device,observed]:devices){
            (void)device;
            if(observed.conflict){result=missing("Conflicting capacities for aliases of the same filesystem device");break;}
            if(!observed.result.value){result=observed.result;break;}
            if(*observed.result.value>std::numeric_limits<std::uint64_t>::max()-total){result=missing("Mounted filesystem capacity sum exceeds uint64 range");break;}
            total+=*observed.result.value;
            result={total,Support::estimated,"Sum of distinct mapped mounted filesystem devices visible to this caller; excludes unmounted/unsupported/hidden filesystems; not total formatted disk capacity"};
        }
        output.emplace(disk,std::move(result));
    }
    return {std::move(output),Support::estimated,"Mapped mounted-filesystem capacity only, not full formatted capacity"};
}
#ifdef __linux__
Metric<std::uint64_t> read_linux_mount_capacity(const LinuxCapacityMount& mount){
    if(mount.path.empty()||mount.path.front()!='/'||mount.path.find('\0')!=mount.path.npos||!device_id(mount.device))return missing("Invalid filesystem query identity");
    const int fd=open(mount.path.c_str(),O_PATH|O_CLOEXEC);
    if(fd<0)return {{},errno==EACCES||errno==EPERM?Support::permission_required:Support::unsupported,"Cannot open filesystem mount reference"};
    struct Owned {int fd;~Owned(){close(fd);}} owned{fd};
    struct stat identity{};struct statvfs stats{};
    if(fstat(fd,&identity)!=0)return missing("Cannot read mount reference identity");
    const auto actual=std::to_string(major(identity.st_dev))+":"+std::to_string(minor(identity.st_dev));
    if(actual!=*device_id(mount.device))return missing("Mounted filesystem device changed or path is overmounted");
    if(fstatvfs(fd,&stats)!=0)return {{},errno==EACCES||errno==EPERM?Support::permission_required:Support::unsupported,"Cannot read filesystem capacity from pinned reference"};
    return linux_filesystem_capacity(stats.f_blocks,stats.f_frsize);
}
Metric<std::string> read_linux_mountinfo(){
    std::ifstream input("/proc/self/mountinfo",std::ios::binary);
    if(!input)return {{},Support::unsupported,"Mount inventory unavailable"};
    std::string text;std::array<char,4096> bytes{};
    while(input){input.read(bytes.data(),bytes.size());const auto count=static_cast<std::size_t>(input.gcount());if(count>16*1024*1024-text.size())return {{},Support::unsupported,"Mount inventory exceeds 16 MiB"};text.append(bytes.data(),count);}
    if(!input.eof())return {{},Support::unsupported,"Mount inventory read failed"};
    return {std::move(text),Support::available,"Caller mount namespace inventory"};
}
std::optional<std::string> resolve_linux_block_device(const std::string& device){
        if(!device_id(device))return {};
        std::error_code error;auto path=std::filesystem::canonical("/sys/dev/block/"+device,error);if(error)return {};
        const bool partition=std::filesystem::exists(path/"partition",error);if(error)return {};
        if(partition)path=path.parent_path();
        const auto name=path.filename().string();
        const auto inventory=std::filesystem::canonical(std::filesystem::path("/sys/block")/name,error);
        if(error||inventory!=path)return {};
        return name;
}
Metric<LinuxFormattedMap> read_native_linux_formatted_capacity(){
    const auto inventory=read_linux_mountinfo();
    if(!inventory.value)return {{},inventory.support,inventory.note};
    LinuxFormattedApi api;
    api.disk=resolve_linux_block_device;
    api.capacity=read_linux_mount_capacity;
    return aggregate_linux_formatted_capacity(*inventory.value,api);
}
#else
Metric<std::string> read_linux_mountinfo(){return {{},Support::unsupported,"Linux mount inventory unsupported on this platform"};}
std::optional<std::string> resolve_linux_block_device(const std::string&){return {};}
Metric<std::uint64_t> read_linux_mount_capacity(const LinuxCapacityMount&){return missing("Linux mount capacity unsupported on this platform");}
Metric<LinuxFormattedMap> read_native_linux_formatted_capacity(){return {{},Support::unsupported,"Linux mount inventory unsupported on this platform"};}
#endif
}
