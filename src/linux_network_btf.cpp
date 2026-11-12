#include "linux_network_btf.hpp"
#include <array>
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string_view>
#ifdef __linux__
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
namespace pcinsight::detail {
namespace {
constexpr std::size_t max_blob=64*1024*1024,max_types=1000000;
struct Header {std::uint16_t magic;std::uint8_t version,flags;std::uint32_t length,type_offset,type_length,string_offset,string_length;};
static_assert(sizeof(Header)==24);
struct Type {std::uint32_t name,info,value;};
struct Member {std::uint32_t name,type,offset;};
struct Parameter {std::uint32_t name,type;};
struct Entry {Type type;std::size_t body;};
template<class T> T load(std::span<const std::byte> data,std::size_t offset){
    if(offset>data.size()||sizeof(T)>data.size()-offset)throw std::runtime_error("Truncated BTF object");
    T value{};std::memcpy(&value,data.data()+offset,sizeof(value));return value;
}
unsigned kind(const Type& t){return (t.info>>24)&31;}
unsigned count(const Type& t){return t.info&65535;}
bool flag(const Type& t){return (t.info>>31)!=0;}
struct Field {std::uint32_t type,offset;};
class Btf {
    std::span<const std::byte> types_,strings_;
    std::vector<Entry> entries_{{}}; // ID0 is void, not a wire type.
public:
    explicit Btf(std::span<const std::byte> blob){
        if(blob.size()>max_blob)throw std::runtime_error("BTF source exceeds64MiB");
        const auto h=load<Header>(blob,0);
        if(h.magic!=0xeb9f||h.version!=1||h.flags||h.length<sizeof(Header)||h.length>blob.size())throw std::runtime_error("Unsupported BTF version/flags/endianness/header");
        const auto base=blob.subspan(h.length);
        auto section=[&](std::size_t offset,std::size_t size){if(offset>base.size()||size>base.size()-offset)throw std::runtime_error("BTF section outside source");return base.subspan(offset,size);};
        types_=section(h.type_offset,h.type_length);strings_=section(h.string_offset,h.string_length);
        if(strings_.empty()||strings_.front()!=std::byte{}||strings_.back()!=std::byte{}||
            (static_cast<std::uint64_t>(h.type_offset)<static_cast<std::uint64_t>(h.string_offset)+h.string_length&&
             static_cast<std::uint64_t>(h.string_offset)<static_cast<std::uint64_t>(h.type_offset)+h.type_length))throw std::runtime_error("Invalid/overlapping BTF sections or string table");
        std::size_t offset=0;
        while(offset<types_.size()){
            const auto t=load<Type>(types_,offset);offset+=sizeof(Type);
            if(entries_.size()>max_types||(t.info&0x60ff0000u))throw std::runtime_error("Excessive BTF types/reserved info bits");
            std::size_t size=0;const auto n=count(t);
            switch(kind(t)){
                case 1:case 14:case 17:size=4;break;
                case 3:size=12;break;
                case 4:case 5:case 15:case 19:size=12*n;break;
                case 6:case 13:size=8*n;break;
                case 2:case 7:case 8:case 9:case 10:case 11:case 12:case 16:case 18:break;
                default:throw std::runtime_error("Unknown BTF kind; cannot safely skip its payload");
            }
            if(size>types_.size()-offset)throw std::runtime_error("Truncated BTF kind payload");
            (void)name(t.name);entries_.push_back({t,offset});offset+=size;
        }
        // Check IDs and referenced names, including unrelated records; no unchecked
        // pointer arithmetic from a wire ID is allowed later in resolution.
        for(std::size_t id=1;id<entries_.size();++id){
            const auto& e=entries_[id];const auto k=kind(e.type),n=count(e.type);
            if(k==2||k==8||k==9||k==10||k==11||k==12||k==13||k==14||k==17||k==18)reference(e.type.value);
            if(k==3){reference(load<std::uint32_t>(types_,e.body));reference(load<std::uint32_t>(types_,e.body+4));}
            if(k==4||k==5)for(unsigned i=0;i<n;++i){const auto m=load<Member>(types_,e.body+12*i);(void)name(m.name);reference(m.type);}
            if(k==13)for(unsigned i=0;i<n;++i){const auto p=load<Parameter>(types_,e.body+8*i);(void)name(p.name);reference(p.type);}
            if(k==6||k==19)for(unsigned i=0;i<n;++i)(void)name(load<std::uint32_t>(types_,e.body+(k==6?8:12)*i));
            if(k==15)for(unsigned i=0;i<n;++i)reference(load<std::uint32_t>(types_,e.body+12*i));
        }
    }
    std::string_view name(std::uint32_t offset) const{
        if(offset>=strings_.size())throw std::runtime_error("BTF string offset outside table");
        const auto* p=reinterpret_cast<const char*>(strings_.data()+offset);
        const auto left=strings_.size()-offset;const auto size=std::min<std::size_t>(left,4097);
        const auto* end=static_cast<const char*>(std::memchr(p,0,size));
        if(!end)throw std::runtime_error("Unterminated/excessive BTF name");
        return {p,static_cast<std::size_t>(end-p)};
    }
    void reference(std::uint32_t id) const{if(id>=entries_.size())throw std::runtime_error("BTF type reference outside table");}
    const Entry& entry(std::uint32_t id) const{reference(id);if(!id)throw std::runtime_error("Unexpected BTF void type");return entries_[id];}
    std::uint32_t unwrap(std::uint32_t id) const{
        for(unsigned depth=0;depth<32;++depth){
            if(!id)return 0; // void pointee, never a scalar/member type
            const auto& t=entry(id).type;const auto k=kind(t);
            if(k!=8&&k!=9&&k!=10&&k!=11&&k!=18)return id;
            id=t.value;
        }
        throw std::runtime_error("BTF qualifier cycle/depth exceeds32");
    }
    std::uint32_t named(unsigned expected,std::string_view text,bool optional=false) const{
        std::uint32_t found=0;
        for(std::size_t i=1;i<entries_.size();++i)if(kind(entries_[i].type)==expected&&name(entries_[i].type.name)==text){
            if(found)throw std::runtime_error("Ambiguous BTF named type/function: "+std::string(text));
            found=static_cast<std::uint32_t>(i);
        }
        if(!found&&!optional)throw std::runtime_error("Required BTF type/function absent: "+std::string(text));
        return found;
    }
    void integer(std::uint32_t id,unsigned bytes,bool is_signed) const{
        const auto& e=entry(unwrap(id));const auto encoded=kind(e.type)==1?load<std::uint32_t>(types_,e.body):0;
        if(kind(e.type)!=1||e.type.value!=bytes||count(e.type)||flag(e.type)||encoded!=((is_signed?1u<<24:0)|bytes*8))
            throw std::runtime_error("Unexpected BTF integer width/encoding/sign");
    }
    void pointer(std::uint32_t id,std::uint32_t pointee) const{
        const auto& t=entry(unwrap(id)).type;
        if(kind(t)!=2||count(t)||flag(t)||unwrap(t.value)!=pointee)throw std::runtime_error("BTF pointer target mismatch");
    }
    Field field(std::uint32_t id,std::string_view text,unsigned bytes) const{
        const auto& e=entry(unwrap(id));if(kind(e.type)!=4||bytes>e.type.value)throw std::runtime_error("Expected complete BTF struct");
        std::optional<Field> found;
        for(unsigned i=0;i<count(e.type);++i){
            const auto m=load<Member>(types_,e.body+12*i);if(name(m.name)!=text)continue;
            if(found)throw std::runtime_error("Duplicate BTF target member");
            const auto bits=flag(e.type)?m.offset&0xffffff:m.offset;
            if((flag(e.type)&&(m.offset>>24))||bits%8||bits/8>e.type.value-bytes)throw std::runtime_error("Target BTF member is bitfield/unaligned/outside struct");
            found=Field{m.type,bits/8};
        }
        if(!found)throw std::runtime_error("Required BTF member absent: "+std::string(text));
        return *found;
    }
    void function(std::uint32_t id,std::uint32_t socket,std::uint32_t message,bool receive) const{
        const auto& f=entry(id).type;
        if(kind(f)!=12||count(f)>1||flag(f))throw std::runtime_error("BTF function linkage not attachable definition");
        const auto& proto=entry(f.value);
        if(kind(proto.type)!=13||count(proto.type)!=(receive?3u:2u)||flag(proto.type))throw std::runtime_error("Unsupported network hook argument count/prototype");
        integer(proto.type.value,4,true);
        pointer(load<Parameter>(types_,proto.body).type,socket);
        pointer(load<Parameter>(types_,proto.body+8).type,message);
        if(receive)integer(load<Parameter>(types_,proto.body+16).type,4,true);
    }
    std::uint32_t taskstats_exit_hook() const{
        const auto task=named(4,"task_struct"),hook=named(12,"taskstats_exit");
        const auto& f=entry(hook).type;const auto& proto=entry(f.value);
        if(count(f)>1||flag(f)||kind(proto.type)!=13||count(proto.type)!=2||flag(proto.type)||proto.type.value)
            throw std::runtime_error("Expected taskstats_exit(task_struct*,int)->void");
        pointer(load<Parameter>(types_,proto.body).type,task);integer(load<Parameter>(types_,proto.body+8).type,4,true);
        return hook;
    }
    LinuxStartupResourceBtfLayout startup_resources() const{
        const auto task=named(4,"task_struct"),sig=named(4,"signal_struct"),cred=named(4,"cred");
        LinuxStartupResourceBtfLayout l;l.function_id=named(12,"release_task");
        const auto& f=entry(l.function_id).type;const auto& p=entry(f.value);
        if(count(f)>1||flag(f)||kind(p.type)!=13||count(p.type)!=1||flag(p.type)||p.type.value)
            throw std::runtime_error("Expected release_task(task_struct*)->void");
        pointer(load<Parameter>(types_,p.body).type,task);
        auto scalar=[&](unsigned type,const char* name,unsigned bytes,bool sign=false){auto v=field(type,name,bytes);integer(v.type,bytes,sign);return v.offset;};
        auto nested=[&](unsigned type,const char* outer,const char* inner,unsigned bytes,bool sign=false){
            const auto v=field(type,outer,1);(void)field(type,outer,entry(unwrap(v.type)).type.value);
            const auto offset=std::uint64_t{v.offset}+scalar(v.type,inner,bytes,sign);
            if(offset>1048576)throw std::runtime_error("Nested accounting offset exceeds bound");
            return static_cast<std::uint32_t>(offset);
        };
        l.tgid=scalar(task,"tgid",4,true);l.start=scalar(task,"start_boottime",8);
        auto v=field(task,"group_leader",8);pointer(v.type,task);l.leader=v.offset;
        v=field(task,"cred",8);pointer(v.type,cred);l.cred=v.offset;l.uid=nested(cred,"uid","val",4);
        v=field(task,"signal",8);pointer(v.type,sig);l.signal=v.offset;
        l.nr_threads=scalar(sig,"nr_threads",4,true);l.live=nested(sig,"live","counter",4,true);
        l.task_runtime=nested(task,"se","sum_exec_runtime",8);l.signal_runtime=scalar(sig,"sum_sched_runtime",8);
        l.task_read=nested(task,"ioac","read_bytes",8);l.task_write=nested(task,"ioac","write_bytes",8);
        l.signal_read=nested(sig,"ioac","read_bytes",8);l.signal_write=nested(sig,"ioac","write_bytes",8);
        for(auto offset:{l.tgid,l.leader,l.start,l.cred,l.uid,l.signal,l.nr_threads,l.live,l.task_runtime,l.signal_runtime,l.task_read,l.task_write,l.signal_read,l.signal_write})
            if(offset>1048576)throw std::runtime_error("Accounting offset exceeds bound");
        return l;
    }
    LinuxBirthPidBtfLayout birth_pid() const{
        const auto task=named(4,"task_struct"),pid=named(4,"pid"),upid=named(4,"upid"),ns=named(4,"pid_namespace"),common=named(4,"ns_common");
        LinuxBirthPidBtfLayout l;
        auto f=field(task,"thread_pid",8);pointer(f.type,pid);l.thread_pid=f.offset;
        f=field(pid,"level",4);integer(f.type,4,false);l.level=f.offset;
        l.upid_size=entry(upid).type.value;
        if(l.upid_size<8||l.upid_size>256)throw std::runtime_error("Unexpected upid stride");
        f=field(upid,"nr",4);integer(f.type,4,true);l.nr=f.offset;
        f=field(upid,"ns",8);pointer(f.type,ns);l.ns=f.offset;
        f=field(ns,"ns",entry(common).type.value);
        if(unwrap(f.type)!=common)throw std::runtime_error("PID namespace common type mismatch");
        auto inode=field(common,"inum",4);integer(inode.type,4,false);
        const auto offset=std::uint64_t{f.offset}+inode.offset;
        if(offset>1048576)throw std::runtime_error("PID namespace inode offset exceeds bound");
        l.ns_inum=static_cast<std::uint32_t>(offset);
        f=field(pid,"numbers",0);l.numbers=f.offset;
        const auto& a=entry(unwrap(f.type));
        if(kind(a.type)!=3||count(a.type)||flag(a.type)||a.type.value||unwrap(load<std::uint32_t>(types_,a.body))!=upid)
            throw std::runtime_error("Expected pid.numbers upid array");
        const auto index=load<std::uint32_t>(types_,a.body+4);
        try{integer(index,4,true);}catch(const std::exception&){integer(index,4,false);}
        const auto n=load<std::uint32_t>(types_,a.body+8);
        if(n>1||std::uint64_t{l.numbers}+std::uint64_t{n}*l.upid_size!=entry(pid).type.value)
            throw std::runtime_error("Expected trailing flexible/one-element pid.numbers array");
        for(auto v:{l.thread_pid,l.level,l.numbers,l.nr,l.ns,l.ns_inum})if(v>1048576)throw std::runtime_error("PID offset exceeds bound");
        return l;
    }
    std::optional<Field> anonymous_field(std::uint32_t id,std::string_view text,unsigned bytes,unsigned depth,unsigned& budget)const{
        if(depth>8||!budget--)throw std::runtime_error("Anonymous BTF traversal exceeds bound");
        id=unwrap(id);const auto& e=entry(id);if(kind(e.type)!=4)throw std::runtime_error("Anonymous BTF member is not a struct");
        std::optional<Field> result;
        for(unsigned n=0;n<count(e.type);++n){const auto m=load<Member>(types_,e.body+12*n);const auto label=name(m.name);
            if(label==text){const auto f=field(id,text,bytes);if(result)throw std::runtime_error("Ambiguous anonymous BTF member");result=f;}
            else if(label.empty()){
                const auto child=unwrap(m.type);if(!child||kind(entry(child).type)!=4)continue;
                const auto bits=flag(e.type)?m.offset&0xffffff:m.offset;
                const auto size=entry(child).type.value;
                if((flag(e.type)&&(m.offset>>24))||bits%8||size>e.type.value||bits/8>e.type.value-size)throw std::runtime_error("Anonymous BTF struct outside parent");
                const auto nested=anonymous_field(child,text,bytes,depth+1,budget);if(!nested)continue;
                const auto offset=std::uint64_t{bits/8}+nested->offset;if(result||offset>1048576)throw std::runtime_error("Ambiguous/excessive anonymous BTF member");
                result=Field{nested->type,static_cast<std::uint32_t>(offset)};
            }
        }
        return result;
    }
    Field exec_field(unsigned type,const char* name,unsigned bytes)const{unsigned budget=256;const auto f=anonymous_field(type,name,bytes,0,budget);if(!f)throw std::runtime_error("Required exec BTF member absent: "+std::string(name));return *f;}
    LinuxBirthExecBtfLayout birth_exec()const{
        const auto task=named(4,"task_struct"),mm=named(4,"mm_struct"),file=named(4,"file"),inode=named(4,"inode"),sb=named(4,"super_block");
        auto ptr=[&](unsigned t,const char* name,unsigned target){const auto f=exec_field(t,name,8);pointer(f.type,target);return f.offset;};
        auto scalar=[&](unsigned t,const char* name,unsigned bytes){const auto f=exec_field(t,name,bytes);integer(f.type,bytes,false);return f.offset;};
        LinuxBirthExecBtfLayout l{ptr(task,"mm",mm),ptr(mm,"exe_file",file),ptr(file,"f_inode",inode),scalar(inode,"i_ino",8),ptr(inode,"i_sb",sb),scalar(sb,"s_dev",4)};
        for(auto n:{l.task_mm,l.exe_file,l.file_inode,l.inode_number,l.inode_sb,l.sb_device})if(n>1048576)throw std::runtime_error("Exec filesystem offset exceeds bound");
        return l;
    }
    std::uint32_t startup_birth_hook() const{
        const auto task=named(4,"task_struct"),hook=named(12,"wake_up_new_task");
        const auto& f=entry(hook).type;const auto& p=entry(f.value);
        if(count(f)>1||flag(f)||kind(p.type)!=13||count(p.type)!=1||flag(p.type)||p.type.value)throw std::runtime_error("Expected wake_up_new_task(task_struct*)->void");
        pointer(load<Parameter>(types_,p.body).type,task);return hook;
    }
    LinuxNetworkBtfLayout network() const{
        const auto socket=named(4,"socket"),sock=named(4,"sock"),common=named(4,"sock_common"),message=named(4,"msghdr"),task=named(4,"task_struct");
        LinuxNetworkBtfLayout layout;layout.send_function_id=named(12,"sock_sendmsg");layout.receive_function_id=named(12,"sock_recvmsg");
        function(layout.send_function_id,socket,message,false);function(layout.receive_function_id,socket,message,true);
        layout.sendto_function_id=named(12,"__sys_sendto");
        const auto& sendto=entry(layout.sendto_function_id).type;
        if(count(sendto)>1||flag(sendto))throw std::runtime_error("sendto hook not a function definition");
        const auto& proto=entry(sendto.value);
        if(kind(proto.type)!=13||count(proto.type)!=6||flag(proto.type))throw std::runtime_error("sendto hook prototype/count mismatch");
        integer(proto.type.value,4,true);
        auto argument=[&](unsigned i){return load<Parameter>(types_,proto.body+8*i).type;};
        integer(argument(0),4,true);pointer(argument(1),0);integer(argument(2),8,false);
        integer(argument(3),4,false);pointer(argument(4),named(4,"sockaddr"));integer(argument(5),4,true);
        layout.inet_send_function_id=named(12,"inet_sendmsg",true);
        layout.inet6_send_function_id=named(12,"inet6_sendmsg",true);
        for(const auto id:{layout.inet_send_function_id,layout.inet6_send_function_id})if(id){
            const auto& f=entry(id).type;
            if(count(f)>1||flag(f))throw std::runtime_error("inet send hook not a function definition");
            const auto& p=entry(f.value);
            if(kind(p.type)!=13||count(p.type)!=3||flag(p.type))throw std::runtime_error("inet send hook prototype/count mismatch");
            integer(p.type.value,4,true);pointer(load<Parameter>(types_,p.body).type,socket);
            pointer(load<Parameter>(types_,p.body+8).type,message);integer(load<Parameter>(types_,p.body+16).type,8,false);
        }
        const auto sk=field(socket,"sk",8);pointer(sk.type,sock);layout.socket_sk_offset=sk.offset;
        const auto c=field(sock,"__sk_common",entry(common).type.value);
        if(unwrap(c.type)!=common)throw std::runtime_error("Unexpected __sk_common member type");
        const auto family=field(common,"skc_family",2);integer(family.type,2,false);layout.sock_family_offset=c.offset+family.offset;
        const auto p=field(sock,"sk_protocol",1);const auto width=entry(unwrap(p.type)).type.value;
        if(width!=1&&width!=2)throw std::runtime_error("Unsupported sk_protocol width");
        integer(p.type,width,false);(void)field(sock,"sk_protocol",width);layout.sock_protocol_offset=p.offset;layout.sock_protocol_width=width;
        const auto group=field(task,"group_leader",8);pointer(group.type,task);layout.task_group_leader_offset=group.offset;
        const auto start=field(task,"start_boottime",8);integer(start.type,8,false);layout.task_start_boottime_offset=start.offset;
        const auto flags=field(task,"flags",4);integer(flags.type,4,false);layout.task_flags_offset=flags.offset;
        // Optional: old capability tests can still run without CONFIG_NET_NS.
        // Never substitute current->nsproxy or ns.inum for the socket cookie.
        try{
            const auto net=named(4,"net");
            const auto pnet=field(common,"skc_net",8);
            (void)field(common,"skc_net",entry(unwrap(pnet.type)).type.value);
            const auto pointer_field=field(pnet.type,"net",8);pointer(pointer_field.type,net);
            const auto cookie=field(net,"net_cookie",8);integer(cookie.type,8,false);
            const std::uint64_t offset=std::uint64_t{c.offset}+pnet.offset+pointer_field.offset;
            if(offset>1048576||cookie.offset>1048576)throw std::runtime_error("Socket namespace offsets exceed bound");
            layout.socket_namespace={LinuxSocketNamespaceBtfLayout{static_cast<std::uint32_t>(offset),cookie.offset},Support::available,
                "Validated socket.__sk_common.skc_net.net -> net.net_cookie; not process namespace inode"};
        }catch(const std::exception& e){layout.socket_namespace={{},Support::unsupported,e.what()};}
        try{
            const auto hook=named(12,"security_ptrace_access_check"),cred=named(4,"cred");
            const auto& f=entry(hook).type;const auto& access_proto=entry(f.value);
            if(count(f)>1||flag(f)||kind(access_proto.type)!=13||count(access_proto.type)!=2||flag(access_proto.type))throw std::runtime_error("Unsupported pidfd identity hook prototype/linkage");
            integer(access_proto.type.value,4,true);pointer(load<Parameter>(types_,access_proto.body).type,task);integer(load<Parameter>(types_,access_proto.body+8).type,4,false);
            const auto tgid=field(task,"tgid",4);integer(tgid.type,4,true);
            const auto credentials=field(task,"cred",8);pointer(credentials.type,cred);
            const auto uid=field(cred,"uid",4),value=field(uid.type,"val",4);integer(value.type,4,false);
            (void)field(cred,"uid",entry(unwrap(uid.type)).type.value);
            const std::uint64_t offset=std::uint64_t{uid.offset}+value.offset;
            if(tgid.offset>1048576||credentials.offset>1048576||offset>1048576)throw std::runtime_error("Pidfd identity offsets exceed bound");
            layout.pidfd_identity={LinuxPidfdIdentityBtfLayout{hook,tgid.offset,credentials.offset,static_cast<std::uint32_t>(offset)},Support::available,
                "Validated security_ptrace_access_check(task*,unsigned)->int and task/cred.uid.val layout only"};
        }catch(const std::exception& e){layout.pidfd_identity={{},Support::unsupported,e.what()};}
        return layout;
    }
};
}
Metric<LinuxNetworkBtfLayout> resolve_linux_network_btf(std::span<const std::byte> blob,unsigned pointer_bytes){
    try{
        if(pointer_bytes!=8)throw std::runtime_error("Network tracing layout currently requires64-bit kernel pointers");
        return {Btf(blob).network(),Support::available,"Validated BTF network hook prototypes/member layouts only; BPF capture not loaded/attached and authorization not verified"};
    }catch(const std::exception& e){return {{},Support::unsupported,e.what()};}
}
Metric<std::uint32_t> resolve_linux_taskstats_exit_hook(std::span<const std::byte> blob){
    try{return {Btf(blob).taskstats_exit_hook(),Support::available,"Validated taskstats_exit(task_struct*,int)->void BTF definition; no attachment performed"};}
    catch(const std::exception& e){return {{},Support::unsupported,e.what()};}
}
Metric<LinuxStartupResourceBtfLayout> resolve_linux_startup_resource_btf(std::span<const std::byte> blob,unsigned pointer_bytes){
    try{if(pointer_bytes!=8)throw std::runtime_error("Resource tracing requires64-bit pointers");return {Btf(blob).startup_resources(),Support::available,"Validated direct leader-release accounting schema only; no coverage or attachment guarantee"};}
    catch(const std::exception& e){return {{},Support::unsupported,e.what()};}
}
Metric<std::uint32_t> resolve_linux_startup_birth_hook(std::span<const std::byte> blob){
    try{return {Btf(blob).startup_birth_hook(),Support::available,"Validated wake_up_new_task(task*)->void only; no attachment"};}
    catch(const std::exception& e){return {{},Support::unsupported,e.what()};}
}
Metric<LinuxBirthPidBtfLayout> resolve_linux_birth_pid_btf(std::span<const std::byte> blob,unsigned pointer_bytes){
    try{if(pointer_bytes!=8)throw std::runtime_error("Birth PID mapping requires64-bit pointers");return {Btf(blob).birth_pid(),Support::available,"Validated task/pid/upid/PID-namespace layout; no attachment or mapping guarantee"};}
    catch(const std::exception& e){return {{},Support::unsupported,e.what()};}
}
Metric<LinuxBirthExecBtfLayout> resolve_linux_birth_exec_btf(std::span<const std::byte> blob,unsigned pointer_bytes){
    try{if(pointer_bytes!=8)throw std::runtime_error("Exec tracing requires64-bit pointers");return {Btf(blob).birth_exec(),Support::available,"Validated task/mm/exe_file/inode/superblock schema, not exec success or tracepoint availability"};}
    catch(const std::exception& e){return {{},Support::unsupported,e.what()};}
}
Metric<std::vector<std::byte>> read_linux_kernel_btf_blob(){
#ifdef __linux__
    struct Fd{int n;~Fd(){if(n>=0)close(n);}};
    const Fd file{open("/sys/kernel/btf/vmlinux",O_RDONLY|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK)};
    auto fail=[](int error){return Metric<std::vector<std::byte>>{{},error==EPERM||error==EACCES?Support::permission_required:Support::unsupported,"Cannot read kernel BTF: errno="+std::to_string(error)};};
    if(file.n<0)return fail(errno);
    struct stat before{},after{};if(fstat(file.n,&before)<0)return fail(errno);
    if(!S_ISREG(before.st_mode)||before.st_size<0||static_cast<std::uint64_t>(before.st_size)>max_blob)return {{},Support::unsupported,"Kernel BTF source not a bounded regular binary file"};
    std::vector<std::byte> data;std::array<std::byte,16384> buffer{};unsigned interrupts=0;
    for(;;){
        const auto n=read(file.n,buffer.data(),buffer.size());
        if(n<0){if(errno==EINTR&&++interrupts<=4)continue;return fail(errno);}
        if(!n)break;
        if(static_cast<std::size_t>(n)>max_blob-data.size())return {{},Support::unsupported,"Kernel BTF exceeds64MiB"};
        data.insert(data.end(),buffer.begin(),buffer.begin()+n);
    }
    if(fstat(file.n,&after)<0)return fail(errno);
    if(before.st_dev!=after.st_dev||before.st_ino!=after.st_ino||before.st_size!=after.st_size||before.st_mtim.tv_sec!=after.st_mtim.tv_sec||before.st_mtim.tv_nsec!=after.st_mtim.tv_nsec)
        return {{},Support::unsupported,"Kernel BTF source identity/metadata changed during read"};
    return {std::move(data),Support::available,"Bounded identity-checked kernel BTF blob; no schema/capture guarantee"};
#else
    return {{},Support::unsupported,"Kernel network BTF layout query requires Linux"};
#endif
}
Metric<LinuxNetworkBtfLayout> read_linux_network_btf(){
    const auto blob=read_linux_kernel_btf_blob();if(!blob.value)return {{},blob.support,blob.note};
    return resolve_linux_network_btf(*blob.value,sizeof(void*));
}
}
