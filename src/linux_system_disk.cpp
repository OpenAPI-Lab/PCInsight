#include "linux_system_disk.hpp"
#include "linux_disk_rates.hpp"
#include <algorithm>
#ifdef __linux__
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>
#endif
namespace pcinsight::detail {
namespace {
bool known(const Metric<std::string>& value){return value.value&&(value.support==Support::available||value.support==Support::estimated);}
bool valid_disk(const std::string& name){return !name.empty()&&name.size()<=4096&&name.find_first_of("/\\\0",0,3)==name.npos&&name!="."&&name!="..";}
Metric<std::string> missing(std::string note){return {{},Support::unsupported,std::move(note)};}
}
Metric<std::string> identify_linux_system_disk(std::string_view text,const LinuxSystemDiskApi& api){
    if(!api.root_device||!api.disk)return missing("Incomplete root filesystem provider");
    if(text.size()>16*1024*1024)return missing("Root mount inventory exceeds 16 MiB");
    std::vector<LinuxCapacityMount> candidates;std::size_t rows=0;
    while(!text.empty()){
        const auto newline=text.find('\n');const auto line=text.substr(0,newline);
        if(!line.empty()){
            if(++rows>100000)return missing("Root mount inventory exceeds row limit");
            const auto mount=parse_linux_capacity_mount(line);if(!mount)return missing("Invalid root mount inventory");
            if(mount->path=="/")candidates.push_back(*mount);
        }
        if(newline==text.npos)break;
        text.remove_prefix(newline+1);
    }
    if(candidates.empty())return missing("No root mount row visible to caller");
    const auto root=api.root_device();
    if(!known(root))return {{},root.support==Support::permission_required?Support::permission_required:Support::unsupported,root.note.empty()?"Root reference device unavailable":root.note};
    if(!valid_linux_disk_device(*root.value))return missing("Invalid root device identity");
    bool matched=false;std::string matched_type;
    for(const auto& candidate:candidates)if(candidate.device==*root.value){
        matched=true;
        if(!matched_type.empty()&&matched_type!=candidate.filesystem)return missing("Conflicting root filesystem types in mount inventory");
        matched_type=candidate.filesystem;
        if(!linux_capacity_filesystem_supported(candidate.filesystem))return missing("Root filesystem topology requires a dedicated provider");
    }
    if(!matched)return missing("Root reference and mount inventory disagree; resample");
    const auto disk=api.disk(*root.value);
    if(!disk||!valid_disk(*disk))return missing("Root filesystem cannot be associated with a visible single block device");
    return {*disk,Support::estimated,"Block device directly associated with caller's mounted root filesystem; not EFI/bootloader or underlying RAID/physical disks"};
}
void apply_linux_system_disk_flags(std::vector<DiskInfo>& disks,const Metric<std::string>& root){
    const bool valid=known(root)&&valid_disk(*root.value);
    const auto matches=valid?std::count_if(disks.begin(),disks.end(),[&](const auto& disk){return disk.id==*root.value;}):0;
    for(auto& disk:disks){
        if(!valid)disk.system_disk={{},root.support==Support::permission_required?Support::permission_required:Support::unsupported,root.note.empty()?"Root filesystem association unavailable":root.note};
        else if(matches!=1)disk.system_disk={{},Support::unsupported,"Root block device missing or ambiguous in disk inventory; resample"};
        else if(!valid_disk(disk.id))disk.system_disk={{},Support::unsupported,"Invalid disk inventory identity"};
        else disk.system_disk={disk.id==*root.value,Support::estimated,root.note};
    }
}
#ifdef __linux__
Metric<std::string> read_linux_root_device(){
    const int fd=open("/",O_PATH|O_CLOEXEC);
    if(fd<0)return {{},errno==EACCES||errno==EPERM?Support::permission_required:Support::unsupported,"Cannot open caller root filesystem reference"};
    struct Owned{int fd;~Owned(){close(fd);}} owned{fd};
    struct stat identity{};
    if(fstat(fd,&identity)!=0)return missing("Cannot read caller root reference identity");
    return {std::to_string(major(identity.st_dev))+":"+std::to_string(minor(identity.st_dev)),Support::available,"st_dev from pinned caller root reference"};
}
Metric<std::string> read_native_linux_system_disk(){
    const auto inventory=read_linux_mountinfo();if(!inventory.value)return {{},inventory.support,inventory.note};
    return identify_linux_system_disk(*inventory.value,{read_linux_root_device,resolve_linux_block_device});
}
#else
Metric<std::string> read_linux_root_device(){return missing("Linux root device unsupported on this platform");}
Metric<std::string> read_native_linux_system_disk(){return missing("Linux system disk unsupported on this platform");}
#endif
}
