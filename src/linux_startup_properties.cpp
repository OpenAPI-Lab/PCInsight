#include "linux_startup_properties.hpp"
#include "linux_startup.hpp"
#include "linux_startup_inventory.hpp"
#include "windows_startup_properties.hpp"
#include <filesystem>
#ifdef __linux__
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
namespace pcinsight::detail {
namespace {
std::string_view trim(std::string_view s) {
    while(!s.empty() && (s.front()==' ' || s.front()=='\t'))s.remove_prefix(1);
    while(!s.empty() && (s.back()==' ' || s.back()=='\t'))s.remove_suffix(1);
    return s;
}
}
std::optional<StartupProperties> parse_startup_properties(std::string_view text) {
    auto entry=read_xdg_startup(text);if(!entry)return {};
    StartupProperties result;result.entry=std::move(*entry);result.file_size_bytes=text.size();
    result.source_file_size_bytes={static_cast<std::uint64_t>(text.size()),Support::available,"Source .desktop size, not executable size"};
    std::string group;
    while(!text.empty()) {
        const auto end=text.find('\n');auto line=text.substr(0,end);
        if(!line.empty() && line.back()=='\r')line.remove_suffix(1);
        line=trim(line);
        if(!line.empty() && line.front()!='#') {
            if(line.front()=='[') {
                if(line.size()>4098 || line.back()!=']')return {};
                group=line.substr(1,line.size()-2);
            } else {
                const auto equals=line.find('=');
                if(equals==line.npos || group.empty())return {};
                const auto key=trim(line.substr(0,equals));const auto value=trim(line.substr(equals+1));
                if(key.empty() || key.size()>4096 || value.size()>32768 || result.fields.size()>=65536)return {};
                result.fields.push_back({group,std::string(key),std::string(value)});
            }
        }
        if(end==text.npos)break;
        text.remove_prefix(end+1);
    }
    return result;
}
}
namespace pcinsight {
namespace {
StartupPropertiesResult failure(ActionError error,const char* message) {return {std::nullopt,error,message};}
#ifdef __linux__
StartupPropertiesResult native_error(const char* message) {
    const auto code=errno;
    return failure(code==ENOENT?ActionError::not_found:(code==EACCES||code==EPERM)?ActionError::permission_denied:ActionError::system_error,message);
}
bool same(const struct stat& a,const struct stat& b) {
    return a.st_dev==b.st_dev && a.st_ino==b.st_ino && a.st_size==b.st_size &&
        a.st_mtim.tv_sec==b.st_mtim.tv_sec && a.st_mtim.tv_nsec==b.st_mtim.tv_nsec &&
        a.st_ctim.tv_sec==b.st_ctim.tv_sec && a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
}
#endif
}
StartupPropertiesResult get_startup_properties(const std::string& id) {
    if(id.empty() || id.size()>262160 || id.find('\0')!=id.npos)
        return failure(ActionError::invalid_argument,"invalid/excessive startup identifier");
#ifdef __linux__
    if(id.size()>32768)return failure(ActionError::invalid_argument,"excessive XDG identifier");
    namespace fs=std::filesystem;
    const auto path=fs::path(id).lexically_normal();
    if(!path.is_absolute() || path.extension()!=".desktop")return failure(ActionError::invalid_argument,"startup requires absolute .desktop identifier");
    try {
        const auto dirs=detail::native_xdg_autostart_directories();
        bool recognized=false;std::string source;
        for(const auto& dir:dirs)if(dir.path==path.parent_path()) {
            recognized=true;source=dir.personal?"XDG effective personal":"XDG effective system";
        }
        if(!recognized)return failure(ActionError::invalid_argument,"startup identifier outside caller XDG context");
        const auto inventory=detail::read_xdg_startup_inventory(dirs);
        if(!inventory.complete)return failure(ActionError::system_error,"startup precedence cannot be verified; reload after fixing directory access");
        const StartupEntry* selected{};
        for(const auto& entry:inventory.entries)if(fs::path(entry.id).filename()==path.filename()){selected=&entry;break;}
        if(!selected)return failure(ActionError::not_found,"startup entry not present in effective inventory");
        if(fs::path(selected->id)!=path)return failure(ActionError::stale_target,"startup ID is shadowed; reload effective inventory");
        struct Fd {int fd;~Fd(){if(fd>=0)close(fd);}} file{open(path.c_str(),O_RDONLY|O_CLOEXEC|O_NONBLOCK)};
        if(file.fd<0)return native_error("startup file cannot be opened");
        struct stat before{},after{};
        if(fstat(file.fd,&before)!=0)return native_error("startup file stat failed");
        if(!S_ISREG(before.st_mode) || before.st_size<0 || before.st_size>1048576)
            return failure(ActionError::invalid_argument,"startup properties requires regular file up to 1 MiB");
        std::string text;char buffer[4096];
        for(;;) {
            const auto n=read(file.fd,buffer,sizeof buffer);
            if(n<0){if(errno==EINTR)continue;return native_error("startup property read failed");}
            if(!n)break;
            if(static_cast<std::size_t>(n)>1048576-text.size())return failure(ActionError::invalid_argument,"startup property payload exceeded limit");
            text.append(buffer,static_cast<std::size_t>(n));
        }
        if(fstat(file.fd,&after)!=0)return native_error("startup revalidation failed");
        if(!same(before,after) || text.size()!=static_cast<std::size_t>(before.st_size) || stat(path.c_str(),&after)!=0 || !same(before,after))
            return failure(ActionError::stale_target,"startup changed during properties query");
        auto properties=detail::parse_startup_properties(text);
        if(!properties)return failure(ActionError::invalid_argument,"startup properties malformed or exceed field limits");
        properties->entry.id=path.string();properties->entry.source=std::move(source);
        if(properties->entry.name.empty())properties->entry.name=path.stem().string();
        const auto context=detail::native_xdg_eligibility_context();
        properties->entry.xdg_standard_eligible=detail::xdg_standard_eligibility(text,context);
        properties->entry.autostart_condition_satisfied=detail::autostart_condition_satisfied(text,context);
        // Confirm priority again: a new personal override must not leave a system
        // properties panel silently tied to a different effective application.
        const auto final_inventory=detail::read_xdg_startup_inventory(dirs);
        if(!final_inventory.complete)return failure(ActionError::stale_target,"startup precedence changed during properties query");
        bool effective=false;
        for(const auto& entry:final_inventory.entries)if(fs::path(entry.id).filename()==path.filename())effective=fs::path(entry.id)==path;
        if(!effective || stat(path.c_str(),&after)!=0 || !same(before,after))
            return failure(ActionError::stale_target,"startup identity or precedence changed; reload inventory");
        return {std::move(properties),ActionError::none,"effective startup properties read; no command executed"};
    } catch(const std::invalid_argument&) {
        return failure(ActionError::invalid_argument,"invalid/excessive XDG context");
    }
#elif defined(_WIN32)
    return detail::get_windows_startup_properties(id);
#else
    return failure(ActionError::not_supported,"startup properties provider requires Linux XDG");
#endif
}
}
