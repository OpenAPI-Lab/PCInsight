#include "linux_privileged_connection.hpp"
#include <atomic>
#include <cstring>
#include <limits>
#include <stdexcept>
#ifdef __linux__
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>
#endif
namespace pcinsight::detail {
namespace {
template<class T>Metric<T> no(std::string note){return {{},Support::unsupported,std::move(note)};}
bool component(const std::string& s){if(s.empty()||s.size()>255||s=="."||s=="..")return false;for(const auto c:s)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c=='.'))return false;return true;}
#ifdef __linux__
struct Pin {int fd{-1};struct stat before{};explicit Pin(int value):fd(value){if(fd<0||fstat(fd,&before)){if(fd>=0)close(fd);fd=-1;throw std::runtime_error("Cannot pin bootstrap directory");}}~Pin(){if(fd>=0)close(fd);}Pin(Pin&& p)noexcept:fd(p.fd),before(p.before){p.fd=-1;}Pin(const Pin&)=delete;};
bool same(const struct stat& a,const struct stat& b){return a.st_dev==b.st_dev&&a.st_ino==b.st_ino&&a.st_mode==b.st_mode&&a.st_uid==b.st_uid&&a.st_gid==b.st_gid;}
bool directory_ok(const struct stat& st,std::uint32_t owner,bool last,bool sticky){return S_ISDIR(st.st_mode)&&(last?st.st_uid==owner:(st.st_uid==0||st.st_uid==owner))&&!(st.st_mode&06000)&&(!(st.st_mode&0022)||(!last&&sticky&&(st.st_mode&S_ISVTX)))&&(!last||!(st.st_mode&S_ISVTX));}
struct Bus;struct Message;
struct Api {
    void* lib=dlopen("libsystemd.so.0",RTLD_NOW|RTLD_LOCAL);
    int (*create)(Bus**){};Bus* (*unref)(Bus*){};void (*close)(Bus*){};
    int (*address)(Bus*,const char*){},(*client)(Bus*,int){},(*fds)(Bus*,int){},(*anonymous)(Bus*,int){},(*interactive)(Bus*,int){},(*exit_disconnect)(Bus*,int){},(*timeout)(Bus*,std::uint64_t){},(*start)(Bus*){},(*own)(Bus*,const char**) {},(*ready)(Bus*){},(*fd)(Bus*){};
    Bus* (*message_bus)(Message*){};
    Message* (*current)(Bus*){};const char* (*sender)(Message*){};int (*type)(Message*,std::uint8_t*){};
    int (*queued_read)(Bus*,std::uint64_t*){},(*queued_write)(Bus*,std::uint64_t*){};
    Api(){if(!lib)return;
#define L(f,s) f=reinterpret_cast<decltype(f)>(dlsym(lib,s))
        L(create,"sd_bus_new");L(unref,"sd_bus_unref");L(close,"sd_bus_close");L(address,"sd_bus_set_address");L(client,"sd_bus_set_bus_client");L(fds,"sd_bus_negotiate_fds");L(anonymous,"sd_bus_set_anonymous");L(interactive,"sd_bus_set_allow_interactive_authorization");L(exit_disconnect,"sd_bus_set_exit_on_disconnect");L(timeout,"sd_bus_set_method_call_timeout");L(start,"sd_bus_start");L(own,"sd_bus_get_unique_name");L(ready,"sd_bus_is_ready");L(fd,"sd_bus_get_fd");
        L(message_bus,"sd_bus_message_get_bus");
        L(current,"sd_bus_get_current_message");L(sender,"sd_bus_message_get_sender");L(type,"sd_bus_message_get_type");L(queued_read,"sd_bus_get_n_queued_read");L(queued_write,"sd_bus_get_n_queued_write");
#undef L
    }
    explicit operator bool()const{return create&&unref&&close&&address&&client&&fds&&anonymous&&interactive&&exit_disconnect&&timeout&&start&&own&&ready&&fd&&message_bus;}
    ~Api(){if(lib)dlclose(lib);}
};
std::atomic<std::uint64_t> next_generation{1};
std::uint64_t allocate_generation(){auto n=next_generation.load();for(;;){if(n==std::numeric_limits<std::uint64_t>::max())throw std::runtime_error("Connection generation exhausted");if(next_generation.compare_exchange_weak(n,n+1))return n;}}
#endif
}
struct PrivilegedDirectory::Impl {
#ifdef __linux__
    std::vector<Pin> pins;std::vector<std::string> parts;std::string path;struct stat mount_ns{},user_ns{};std::uint32_t owner{};bool sticky{};pid_t pid=getpid();uid_t euid=geteuid();
    bool valid()const{if(pid!=getpid()||euid!=geteuid()||pins.empty())return false;struct stat st{};
        if(stat("/proc/thread-self/ns/mnt",&st)||st.st_dev!=mount_ns.st_dev||st.st_ino!=mount_ns.st_ino||stat("/proc/thread-self/ns/user",&st)||st.st_dev!=user_ns.st_dev||st.st_ino!=user_ns.st_ino)return false;
        for(std::size_t i=0;i<pins.size();++i){if(fstat(pins[i].fd,&st)||!same(pins[i].before,st)||!directory_ok(st,owner,i+1==pins.size(),sticky))return false;if(i&& (fstatat(pins[i-1].fd,parts[i-1].c_str(),&st,AT_SYMLINK_NOFOLLOW)||!same(pins[i].before,st)))return false;}return true;}
#endif
};
PrivilegedDirectory::PrivilegedDirectory(std::unique_ptr<Impl> p):impl_(std::move(p)){}
PrivilegedDirectory::~PrivilegedDirectory()=default;
bool PrivilegedDirectory::revalidate()const{
#ifdef __linux__
    return impl_&&impl_->valid();
#else
    return false;
#endif
}
int PrivilegedDirectory::native_fd()const{
#ifdef __linux__
    return revalidate()?impl_->pins.back().fd:-1;
#else
    return -1;
#endif
}
std::string PrivilegedDirectory::absolute_path()const{
#ifdef __linux__
    return revalidate()?impl_->path:std::string{};
#else
    return {};
#endif
}
Metric<std::shared_ptr<PrivilegedDirectory>> open_privileged_directory(std::string path,std::uint32_t owner,bool sticky){
    if(path.empty()||path[0]!='/'||path.size()>4096||owner==UINT32_MAX)return no<std::shared_ptr<PrivilegedDirectory>>("Invalid absolute directory/owner");
#ifdef __linux__
    try{auto p=std::make_unique<PrivilegedDirectory::Impl>();p->path=path;p->owner=owner;p->sticky=sticky;
        if(stat("/proc/thread-self/ns/mnt",&p->mount_ns)||stat("/proc/thread-self/ns/user",&p->user_ns))throw std::runtime_error("Bootstrap namespace identity unavailable");
        std::size_t at=1;while(at<path.size()){const auto slash=path.find('/',at);auto part=path.substr(at,slash==path.npos?path.size()-at:slash-at);if(!component(part)||p->parts.size()>=32)throw std::runtime_error("Invalid closed directory path");p->parts.push_back(std::move(part));if(slash==path.npos)break;at=slash+1;if(at==path.size())throw std::runtime_error("Trailing directory separator");}
        p->pins.reserve(p->parts.size()+1);p->pins.emplace_back(open("/",O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW));if(!directory_ok(p->pins.back().before,owner,p->parts.empty(),sticky))throw std::runtime_error("Untrusted bootstrap root directory");
        for(std::size_t i=0;i<p->parts.size();++i){p->pins.emplace_back(openat(p->pins.back().fd,p->parts[i].c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK));if(!directory_ok(p->pins.back().before,owner,i+1==p->parts.size(),sticky))throw std::runtime_error("Untrusted bootstrap directory owner/mode");}
        if(!p->valid())throw std::runtime_error("Bootstrap directory changed");
        return {std::shared_ptr<PrivilegedDirectory>(new PrivilegedDirectory(std::move(p))),Support::available,"Pinned absolute directory; trusted process/loader still required"};
    }catch(const std::exception& e){return no<std::shared_ptr<PrivilegedDirectory>>(e.what());}
#else
    (void)sticky;return no<std::shared_ptr<PrivilegedDirectory>>("Bootstrap directories require Linux");
#endif
}
struct PrivilegedBusConnection::Impl {
#ifdef __linux__
    Api api;Bus* bus{};PrivilegedBusEndpoint endpoint;std::string path;struct stat socket{};std::uint64_t epoch{};mutable bool revoked{};pid_t pid=getpid();uid_t euid=geteuid();
    std::vector<std::shared_ptr<PrivilegedNativeJobWatch>> watches;std::string subscribed_owner;
    std::unique_ptr<PrivilegedAdmission> admission;
    ~Impl(){watches.clear();if(bus&&api.close&&api.unref){api.close(bus);api.unref(bus);}}
#endif
};
PrivilegedBusConnection::PrivilegedBusConnection(std::unique_ptr<Impl> p):impl_(std::move(p)){}
PrivilegedBusConnection::~PrivilegedBusConnection()=default;
void* PrivilegedBusConnection::native_handle()const{
#ifdef __linux__
    if(!impl_||impl_->revoked)return nullptr;
    auto revoke=[&]()->void*{impl_->revoked=true;for(auto& w:impl_->watches)w->observation()->close_pending("Owned connection revoked; job not canceled");return nullptr;};
    if(impl_->pid!=getpid()||impl_->euid!=geteuid()||!impl_->bus||impl_->api.ready(impl_->bus)<=0||!impl_->endpoint.directory->revalidate())return revoke();
    struct stat node{};const auto fd=impl_->endpoint.directory->native_fd();if(fd<0||fstatat(fd,impl_->endpoint.socket_name.c_str(),&node,AT_SYMLINK_NOFOLLOW)||!same(impl_->socket,node))return revoke();return impl_->bus;
#else
    return nullptr;
#endif
}
std::uint64_t PrivilegedBusConnection::generation()const{
#ifdef __linux__
    return native_handle()?impl_->epoch:0;
#else
    return 0;
#endif
}
PrivilegedIdentityConfig PrivilegedBusConnection::identity_config(std::uint32_t authority_uid)const{
    PrivilegedIdentityConfig c;c.socket_path.clear();
#ifdef __linux__
    if(native_handle()){c.socket_path=impl_->path;c.bus_uid=impl_->endpoint.bus_peer_uid;c.authority_uid=authority_uid;c.connection_generation=impl_->epoch;}
#else
    (void)authority_uid;
#endif
    return c; // Policy generation is supplied by the managed policy receiver.
}
void PrivilegedBusConnection::disconnect(){impl_.reset();}
Metric<bool> PrivilegedBusConnection::admit_request(void* message)const{
#ifdef __linux__
    auto* bus=static_cast<Bus*>(native_handle());if(!bus||!message||!impl_->admission)return no<bool>("Admission missing owned connection/message");
    auto& a=impl_->api;auto* m=static_cast<Message*>(message);std::uint8_t type{};std::uint64_t read{},write{};
    if(!a.current||!a.sender||!a.type||!a.queued_read||!a.queued_write||a.message_bus(m)!=bus||a.current(bus)!=m||a.type(m,&type)<0||type!=1||a.queued_read(bus,&read)<0||a.queued_write(bus,&write)<0)return no<bool>("Admission unbound dispatch/queue counters unavailable");
    const auto* sender=a.sender(m);if(!sender)return no<bool>("Admission missing broker sender");const auto size=strnlen(sender,256);if(size>255)return no<bool>("Admission invalid sender bound");return impl_->admission->admit(std::string_view(sender,size),read,write);
#else
    (void)message;return no<bool>("Admission requires Linux");
#endif
}
Metric<std::shared_ptr<PrivilegedNativeJobWatch>> PrivilegedBusConnection::prepare_job_watch(std::string owner,std::string unit,std::uint64_t timeout,std::shared_ptr<PrivilegedRequestBudget> budget)const{
    auto no=[](std::string note)->Metric<std::shared_ptr<PrivilegedNativeJobWatch>>{return {{},Support::unsupported,std::move(note)};};
#ifdef __linux__
    const auto bus=native_handle();if(!bus)return no("Owned connection unavailable for job match");
    if(!impl_->subscribed_owner.empty()&&impl_->subscribed_owner!=owner){for(auto& w:impl_->watches)w->observation()->close_pending("Manager owner changed");impl_->watches.clear();impl_->subscribed_owner.clear();}
    std::erase_if(impl_->watches,[](const auto& w){return w->observation()->snapshot().state!=PrivilegedJobState::pending;});
    if(impl_->watches.size()>=64)return no("Pending native job watch bound64 exceeded");
    impl_->watches.reserve(64);auto watched=create_privileged_job_watch(bus,owner,std::move(unit),timeout,impl_->subscribed_owner!=owner,std::move(budget));
    if(metric_is_known(watched)){impl_->subscribed_owner=std::move(owner);impl_->watches.push_back(*watched.value);}return watched;
#else
    (void)owner;(void)unit;(void)timeout;(void)budget;return no("Native job watching requires Linux");
#endif
}
Metric<std::shared_ptr<PrivilegedBusConnection>> connect_privileged_bus(PrivilegedBusEndpoint endpoint){
    if(!endpoint.directory||!component(endpoint.socket_name)||endpoint.socket_owner_uid==UINT32_MAX||endpoint.bus_peer_uid==UINT32_MAX||!endpoint.call_timeout_usec||endpoint.call_timeout_usec>5000000||!valid_privileged_admission_config(endpoint.admission))return no<std::shared_ptr<PrivilegedBusConnection>>("Invalid closed mechanism endpoint/UID/timeout/admission");
#ifdef __linux__
    try{if(!endpoint.directory->revalidate())throw std::runtime_error("Bootstrap directory is stale/untrusted");
        auto p=std::make_unique<PrivilegedBusConnection::Impl>();const auto directory=endpoint.directory->absolute_path();if(directory.empty())throw std::runtime_error("Bootstrap directory lost");p->path=directory+(directory=="/"?"":"/")+endpoint.socket_name;
        if(p->path.size()>107)throw std::runtime_error("Unix socket path bound exceeded");
        p->endpoint=std::move(endpoint);p->admission=std::make_unique<PrivilegedAdmission>(p->endpoint.admission);auto& a=p->api;if(!a||!a.current||!a.sender||!a.type||!a.queued_read||!a.queued_write)throw std::runtime_error("Required native connection/admission symbols unavailable");
        const int parent=p->endpoint.directory->native_fd();struct stat before{},after{};if(parent<0||fstatat(parent,p->endpoint.socket_name.c_str(),&before,AT_SYMLINK_NOFOLLOW)||!S_ISSOCK(before.st_mode)||before.st_uid!=p->endpoint.socket_owner_uid||before.st_nlink!=1)throw std::runtime_error("Untrusted/missing socket node");
        auto ok=[](int rc){if(rc<0)throw std::runtime_error("Explicit native bus connection failed");};ok(a.create(&p->bus));if(!p->bus)throw std::runtime_error("Missing native connection");
        ok(a.address(p->bus,("unix:path="+p->path).c_str()));ok(a.client(p->bus,1));ok(a.fds(p->bus,0));ok(a.anonymous(p->bus,0));ok(a.interactive(p->bus,0));ok(a.exit_disconnect(p->bus,0));ok(a.timeout(p->bus,p->endpoint.call_timeout_usec));ok(a.start(p->bus));const char* own{};ok(a.own(p->bus,&own));if(!own||own[0]!=':'||a.ready(p->bus)<=0)throw std::runtime_error("Bus handshake not ready");
        const int fd=a.fd(p->bus);struct ucred peer{};socklen_t size=sizeof peer;int type{};socklen_t type_size=sizeof type;sockaddr_un address{};socklen_t length=sizeof address;
        const auto fd_flags=fd<0?-1:fcntl(fd,F_GETFD);
        if(fd<0||fd_flags<0||!(fd_flags&FD_CLOEXEC)||getsockopt(fd,SOL_SOCKET,SO_TYPE,&type,&type_size)||type!=SOCK_STREAM||getsockopt(fd,SOL_SOCKET,SO_PEERCRED,&peer,&size)||size!=sizeof peer||peer.pid<=0||peer.uid!=p->endpoint.bus_peer_uid||getpeername(fd,reinterpret_cast<sockaddr*>(&address),&length)||address.sun_family!=AF_UNIX||length<=offsetof(sockaddr_un,sun_path)||address.sun_path[0]=='\0'||std::string(address.sun_path,strnlen(address.sun_path,sizeof address.sun_path))!=p->path)throw std::runtime_error("Native bus peer/UID/address/CLOEXEC mismatch");
        if(!p->endpoint.directory->revalidate()||fstatat(parent,p->endpoint.socket_name.c_str(),&after,AT_SYMLINK_NOFOLLOW)||!same(before,after)||p->pid!=getpid()||p->euid!=geteuid())throw std::runtime_error("Endpoint/process changed during connection");
        p->socket=after;p->epoch=allocate_generation();
        return {std::shared_ptr<PrivilegedBusConnection>(new PrivilegedBusConnection(std::move(p))),Support::available,"Owned explicit bus connection only, no Authority activation or execution; serialize borrowed handle use"};
    }catch(const std::exception& e){return no<std::shared_ptr<PrivilegedBusConnection>>(e.what());}
#else
    return no<std::shared_ptr<PrivilegedBusConnection>>("Native connection requires Linux");
#endif
}
PrivilegedAuthorizationObservation receive_owned_privileged_request(void* message,const PrivilegedBusConnection& connection,std::uint32_t authority,const PrivilegedPolicyStore& policy,bool interaction){
    PrivilegedAuthorizationObservation out;
    auto reject=[&](const char* note){out.state=PrivilegedAuthorizationState::stale;out.authorization=no<bool>(note);out.note=note;};
    const auto handle=connection.native_handle();if(!message||!handle){reject("Missing request or revoked owned connection");return out;}
    const auto admitted=connection.admit_request(message);if(admitted.value!=true){out.state=PrivilegedAuthorizationState::rejected;out.authorization=no<bool>(admitted.note);out.note=admitted.note;return out;}
#ifdef __linux__
    Api api;if(!api||api.message_bus(static_cast<Message*>(message))!=handle){reject("Incoming request belongs to another connection");return out;}
    auto config=connection.identity_config(authority);config.budget=std::make_shared<PrivilegedRequestBudget>();out=receive_native_privileged_policy_request(message,config,policy,interaction);
    if(connection.native_handle()!=handle||connection.generation()!=config.connection_generation)reject("Owned connection/bootstrap changed; observation discarded");
#else
    (void)authority;(void)policy;(void)interaction;reject("Owned receiver requires Linux");
#endif
    return out;
}
}
