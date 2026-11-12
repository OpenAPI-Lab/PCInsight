#include "linux_privileged_policy.hpp"
#include "linux_service_actions.hpp"
#include <limits>
#include <set>
#include <stdexcept>
#ifdef __linux__
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
namespace pcinsight::detail {
namespace {
constexpr std::size_t byte_limit=96*1024;
template<class T>Metric<T> no(std::string note,Support support=Support::unsupported){return {{},support,std::move(note)};}
#ifdef __linux__
struct Pin {
    int fd{-1};struct stat before{};
    explicit Pin(int f):fd(f){if(fd<0||fstat(fd,&before)){if(fd>=0)close(fd);fd=-1;throw std::runtime_error("Cannot pin policy path");}}
    ~Pin(){if(fd>=0)close(fd);}
    Pin(Pin&& other)noexcept:fd(other.fd),before(other.before){other.fd=-1;}
    Pin(const Pin&)=delete;
};
bool same(const struct stat& a,const struct stat& b){return a.st_dev==b.st_dev&&a.st_ino==b.st_ino&&a.st_mode==b.st_mode&&a.st_uid==b.st_uid&&a.st_gid==b.st_gid&&a.st_nlink==b.st_nlink&&a.st_size==b.st_size&&a.st_mtim.tv_sec==b.st_mtim.tv_sec&&a.st_mtim.tv_nsec==b.st_mtim.tv_nsec&&a.st_ctim.tv_sec==b.st_ctim.tv_sec&&a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;}
void protect(const struct stat& s,std::uint32_t owner,bool directory){if(s.st_uid!=owner||(s.st_mode&0022)||(s.st_mode&07000)||(directory?!S_ISDIR(s.st_mode):(!S_ISREG(s.st_mode)||s.st_nlink!=1)))throw std::runtime_error("Untrusted policy owner/mode/type/link count");}
#endif
}
Metric<std::vector<PrivilegedServiceRule>> parse_privileged_policy(std::string_view bytes){
    constexpr std::string_view header="PCINSIGHT-SERVICE-POLICY 1\n";
    if(bytes.size()>byte_limit||!bytes.starts_with(header)||bytes.find('\0')!=bytes.npos||bytes.back()!='\n')return no<std::vector<PrivilegedServiceRule>>("Policy bound/header/NUL/final newline invalid");
    std::vector<PrivilegedServiceRule> rules;std::set<std::string> seen;std::size_t at=header.size();
    while(at<bytes.size()){
        const auto end=bytes.find('\n',at);const auto line=bytes.substr(at,end-at);const auto space=line.find(' ');
        if(rules.size()>=256||space==line.npos||space+2!=line.size()||line.back()<'0'||line.back()>'7')return no<std::vector<PrivilegedServiceRule>>("Policy rule count/closed syntax invalid");
        std::string unit(line.substr(0,space));if(!valid_systemd_service_name(unit)||!seen.insert(unit).second)return no<std::vector<PrivilegedServiceRule>>("Invalid/duplicate literal service rule");
        rules.push_back({std::move(unit),std::uint8_t(line.back()-'0')});at=end+1;
    }
    return {std::move(rules),Support::available,"Closed policy parsed only; no permissions/authentication granted"};
}
Metric<std::vector<PrivilegedServiceRule>> load_privileged_policy(int anchor,std::string_view path,std::uint32_t owner){
    if(anchor<0||owner==std::numeric_limits<std::uint32_t>::max()||path.empty()||path.size()>4096||path[0]=='/'||path.find('\0')!=path.npos)return no<std::vector<PrivilegedServiceRule>>("Invalid trusted anchor/owner/relative path");
    std::vector<std::string> parts;std::size_t at{};
    while(at<=path.size()){
        const auto slash=path.find('/',at);const auto part=path.substr(at,slash==path.npos?path.size()-at:slash-at);
        if(part.empty()||part=="."||part==".."||part.size()>255||parts.size()>=16)return no<std::vector<PrivilegedServiceRule>>("Unsafe/excessive relative policy path");
        for(const auto c:part)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='.'||c=='_'||c=='-'))return no<std::vector<PrivilegedServiceRule>>("Policy path component outside closed alphabet");
        parts.emplace_back(part);if(slash==path.npos)break;at=slash+1;
    }
#ifdef __linux__
    try{
        std::vector<Pin> parents;parents.reserve(parts.size());parents.emplace_back(fcntl(anchor,F_DUPFD_CLOEXEC,3));protect(parents.back().before,owner,true);
        for(std::size_t i=0;i+1<parts.size();++i){const int fd=openat(parents.back().fd,parts[i].c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC|O_NONBLOCK);parents.emplace_back(fd);protect(parents.back().before,owner,true);}
        struct stat named{};const auto& leaf=parts.back();if(fstatat(parents.back().fd,leaf.c_str(),&named,AT_SYMLINK_NOFOLLOW))throw std::runtime_error("Policy file unavailable");protect(named,owner,false);
        Pin file(openat(parents.back().fd,leaf.c_str(),O_RDONLY|O_NOFOLLOW|O_CLOEXEC|O_NONBLOCK));protect(file.before,owner,false);
        if(!same(named,file.before)||file.before.st_size<0||std::uint64_t(file.before.st_size)>byte_limit)throw std::runtime_error("Policy changed/size bound exceeded before read");
        std::string bytes;char chunk[4096];
        for(;;){const auto n=read(file.fd,chunk,sizeof chunk);if(n<0){if(errno==EINTR)continue;throw std::runtime_error("Policy read failed");}if(!n)break;if(bytes.size()+std::size_t(n)>byte_limit)throw std::runtime_error("Policy byte bound exceeded");bytes.append(chunk,std::size_t(n));}
        struct stat after{};if(fstat(file.fd,&after)||!same(file.before,after)||bytes.size()!=std::uint64_t(file.before.st_size)||fstatat(parents.back().fd,leaf.c_str(),&after,AT_SYMLINK_NOFOLLOW)||!same(file.before,after))throw std::runtime_error("Policy changed/replaced during read");
        for(std::size_t i=0;i<parents.size();++i){if(fstat(parents[i].fd,&after)||!same(parents[i].before,after))throw std::runtime_error("Policy directory changed during read");if(i+1<parents.size()&&(fstatat(parents[i].fd,parts[i].c_str(),&after,AT_SYMLINK_NOFOLLOW)||!same(parents[i+1].before,after)))throw std::runtime_error("Policy directory replaced during read");}
        return parse_privileged_policy(bytes);
    }catch(const std::exception& e){return no<std::vector<PrivilegedServiceRule>>(e.what(),Support::permission_required);}catch(...){return no<std::vector<PrivilegedServiceRule>>("Protected policy load failed");}
#else
    return no<std::vector<PrivilegedServiceRule>>("Protected policy loading requires Linux");
#endif
}
void PrivilegedPolicyStore::advance_locked(){
    // Revoke before any diagnostic allocation can throw.
    current_.value.reset();current_.support=Support::unsupported;current_.note.clear();
    if(generation_==std::numeric_limits<std::uint64_t>::max())exhausted_=true;else ++generation_;
    current_.note="Policy invalidated; no stale fallback";
}
void PrivilegedPolicyStore::invalidate(){std::lock_guard guard(state_mutex_);advance_locked();}
Metric<PrivilegedPolicySnapshot> PrivilegedPolicyStore::snapshot()const{std::lock_guard guard(state_mutex_);return current_;}
bool PrivilegedPolicyStore::is_current(std::uint64_t generation)const{std::lock_guard guard(state_mutex_);return !exhausted_&&generation&&generation==generation_&&metric_is_known(current_);}
Metric<PrivilegedPolicySnapshot> PrivilegedPolicyStore::refresh(int anchor,std::string_view path,std::uint32_t owner){
    std::lock_guard refresh_guard(refresh_mutex_);std::uint64_t epoch{};
    {std::lock_guard guard(state_mutex_);advance_locked();if(exhausted_)return no<PrivilegedPolicySnapshot>("Policy generation exhausted; restart trusted mechanism");epoch=generation_;}
    const auto loaded=load_privileged_policy(anchor,path,owner);
    std::lock_guard guard(state_mutex_);
    if(generation_!=epoch||exhausted_)return no<PrivilegedPolicySnapshot>("Policy refresh invalidated while loading");
    if(metric_is_known(loaded))current_={PrivilegedPolicySnapshot{*loaded.value,epoch},Support::available,"Owned protected-policy snapshot; generation is not an execution ticket"};
    else current_=no<PrivilegedPolicySnapshot>(loaded.note,loaded.support);
    return current_;
}
}
