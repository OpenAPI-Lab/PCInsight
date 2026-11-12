#include "linux_process_details.hpp"
#include <array>
#include <charconv>
#include <cerrno>
#include <limits>
#ifdef __linux__
#include <fcntl.h>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
namespace pcinsight::detail {
namespace {
template<class T> Metric<T> absent(std::string note){return {{},Support::unsupported,std::move(note)};}
std::string_view trim(std::string_view s){const auto a=s.find_first_not_of(" \t\r"),b=s.find_last_not_of(" \t\r");return a==s.npos?std::string_view{}:s.substr(a,b-a+1);}
bool number(std::string_view s,std::uint32_t& n){if(s.empty())return false;const auto [p,e]=std::from_chars(s.data(),s.data()+s.size(),n);return e==std::errc{}&&p==s.data()+s.size();}
// ASCII-safe representation of kernel text; not a commercial application label.
std::string display(std::string_view s){std::string out;constexpr char hex[]="0123456789abcdef";for(unsigned char c:s){if(c<32||c>=127){out+="\\x";out+=hex[c>>4];out+=hex[c&15];}else out+=static_cast<char>(c);}return out;}
#ifdef __linux__
struct Fd{int value;~Fd(){if(value>=0)::close(value);}};
Metric<std::string> failure(int error){return {{},error==EACCES||error==EPERM?Support::permission_required:Support::unsupported,"Process details read failed (errno="+std::to_string(error)+")"};}
Metric<std::string> read_at(int dir,const char* file,std::size_t limit){
    const Fd fd{::openat(dir,file,O_RDONLY|O_NOFOLLOW|O_CLOEXEC|O_NONBLOCK)};if(fd.value<0){const int e=errno;return failure(e);}
    std::string text;std::array<char,8192> buffer{};unsigned interrupts=0;
    for(;;){const auto bytes=::read(fd.value,buffer.data(),buffer.size());if(bytes<0){const int e=errno;if(e==EINTR&&++interrupts<=3)continue;return failure(e);}if(!bytes)break;if(static_cast<std::size_t>(bytes)>limit-text.size())return absent<std::string>("Process details source exceeds bound");text.append(buffer.data(),static_cast<std::size_t>(bytes));}
    return {std::move(text),Support::available,"Bounded read through pinned directory"};
}
LinuxExecutableStamp stamp(const struct stat& st){
    return {static_cast<std::uint64_t>(st.st_dev),static_cast<std::uint64_t>(st.st_ino),static_cast<std::uint64_t>(st.st_size),static_cast<std::uint64_t>(st.st_mode),st.st_mtim.tv_sec,st.st_mtim.tv_nsec,st.st_ctim.tv_sec,st.st_ctim.tv_nsec};
}
Metric<std::string> exe_link(int dir){
    std::vector<char> buffer(65537);unsigned interrupts=0;
    for(;;){
        const auto n=::readlinkat(dir,"exe",buffer.data(),buffer.size());
        if(n<0){const int e=errno;if(e==EINTR&&++interrupts<=3)continue;return failure(e);}
        if(!n||static_cast<std::size_t>(n)>=buffer.size())return absent<std::string>("Empty/truncated executable link");
        return {std::string(buffer.data(),static_cast<std::size_t>(n)),Support::estimated,"Observed proc exe link, not a reusable filesystem identity; deleted suffix preserved"};
    }
}
Metric<std::string> architecture_from_fd(int target,const LinuxExecutableStamp& expected){
    // Follow only our own pinned O_PATH descriptor, never reopen by sampled name.
    const auto path="/proc/self/fd/"+std::to_string(target);
    const Fd fd{::open(path.c_str(),O_RDONLY|O_CLOEXEC|O_NONBLOCK)};
    if(fd.value<0){const int e=errno;return failure(e);}
    struct stat before{},after{};
    if(::fstat(fd.value,&before)<0){const int e=errno;return failure(e);}
    if(!S_ISREG(before.st_mode)||before.st_size<0||stamp(before)!=expected)return absent<std::string>("Pinned executable identity differs before header read");
    std::array<unsigned char,64> buffer{};std::size_t used=0;unsigned interrupts=0;
    while(used<buffer.size()){
        const auto n=::pread(fd.value,buffer.data()+used,buffer.size()-used,static_cast<off_t>(used));
        if(n<0){const int e=errno;if(e==EINTR&&++interrupts<=3)continue;return failure(e);}
        if(!n)break;
        used+=static_cast<std::size_t>(n);
    }
    if(::fstat(fd.value,&after)<0){const int e=errno;return failure(e);}
    if(stamp(after)!=expected)return absent<std::string>("Executable metadata changed during header read");
    return linux_executable_architecture(std::span(buffer.data(),used));
}
#endif
}
Metric<LinuxProcessStatus> parse_linux_process_status(std::string_view text,std::uint32_t expected){
    auto bad=[](){return absent<LinuxProcessStatus>("Invalid/duplicate/oversized status field or mismatched PID");};
    if(!expected||text.empty()||text.size()>1024*1024)return bad();
    LinuxProcessStatus out;bool pid_seen=false,uid_seen=false,name_seen=false,state_seen=false;unsigned lines=0;
    while(!text.empty()){
        const auto end=text.find('\n');const auto line=text.substr(0,end);
        if(++lines>10000||line.size()>65536||line.find('\0')!=line.npos)return bad();
        const auto colon=line.find(':');const auto key=colon==line.npos?std::string_view{}:trim(line.substr(0,colon));const auto field=colon==line.npos?std::string_view{}:trim(line.substr(colon+1));
        if(key=="Pid") {if(pid_seen||!number(field,out.pid)||out.pid!=expected)return bad();pid_seen=true;}
        else if(key=="Uid"){
            if(uid_seen)return bad();
            uid_seen=true;auto rest=field;std::array<std::uint32_t,4> ids{};
            for(auto& id:ids){rest=trim(rest);const auto split=rest.find_first_of(" \t\r");if(!number(rest.substr(0,split),id))return bad();rest=split==rest.npos?std::string_view{}:rest.substr(split);}
            if(!trim(rest).empty())return bad();
            out.real_uid=ids[0];
        }else if(key=="Name") {if(name_seen||field.size()>4096)return bad();name_seen=true;out.name=display(field);}
        else if(key=="State"){
            if(state_seen||field.empty()||field.size()>256)return bad();
            state_seen=true;
            if(!((field[0]>='A'&&field[0]<='Z')||(field[0]>='a'&&field[0]<='z'))||(field.size()>1&&field[1]!=' '&&field[1]!='\t'))return bad();
            out.state=display(field);
        }
        if(end==text.npos)break;
        text.remove_prefix(end+1);
    }
    if(!pid_seen)return bad();
    return {out,Support::estimated,"Kernel status snapshot: real UID (not effective owner), kernel Name ASCII-escaped (not application description), State observed only at read time"};
}
Metric<LinuxProcessStatus> linux_process_status_for_identity(const LinuxProcessStat& expected,const std::optional<LinuxProcessStat>& before,const std::optional<LinuxProcessStat>& after,Metric<LinuxProcessStatus> status){
    auto same=[&](const auto& row){return row&&row->pid==expected.pid&&row->start_ticks==expected.start_ticks;};
    if(!expected.pid||!same(before)||!same(after)||(status.value&&status.value->pid!=expected.pid))return absent<LinuxProcessStatus>("Process identity absent/changed around status read");
    return status;
}
Metric<LinuxProcessStat> read_linux_process_identity(const std::filesystem::path& path){
#ifdef __linux__
    const Fd dir{::open(path.c_str(),O_PATH|O_DIRECTORY|O_CLOEXEC)};if(dir.value<0){const int e=errno;const auto f=failure(e);return {{},f.support,f.note};}
    const auto source=read_at(dir.value,"stat",65536);if(!source.value)return {{},source.support,source.note};
    const auto stat=parse_linux_process_stat(*source.value);if(!stat||!stat->pid)return absent<LinuxProcessStat>("Invalid process stat");return {*stat,Support::available,"Bounded stat identity; same boot/namespace only"};
#else
    (void)path;return absent<LinuxProcessStat>("Linux-only source");
#endif
}
Metric<LinuxProcessStatus> read_linux_process_status(const std::filesystem::path& path,const LinuxProcessStat& expected){
    if(!expected.pid)return absent<LinuxProcessStatus>("Invalid expected PID");
#ifdef __linux__
    const Fd dir{::open(path.c_str(),O_PATH|O_DIRECTORY|O_CLOEXEC)};if(dir.value<0){const int e=errno;const auto f=failure(e);return {{},f.support,f.note};}
    const auto first=read_at(dir.value,"stat",65536);if(!first.value)return {{},first.support,first.note};const auto before=parse_linux_process_stat(*first.value);
    if(!before||before->pid!=expected.pid||before->start_ticks!=expected.start_ticks)return absent<LinuxProcessStatus>("Identity differs before status read");
    const auto source=read_at(dir.value,"status",1024*1024);auto status=source.value?parse_linux_process_status(*source.value,expected.pid):Metric<LinuxProcessStatus>{{},source.support,source.note};
    const auto last=read_at(dir.value,"stat",65536);if(!last.value)return {{},last.support,last.note};return linux_process_status_for_identity(expected,before,parse_linux_process_stat(*last.value),std::move(status));
#else
    (void)path;return absent<LinuxProcessStatus>("Linux-only source");
#endif
}
Metric<std::string> linux_username_for_uid(std::uint32_t uid){
#ifdef __linux__
    if(static_cast<std::uint64_t>(uid)>std::numeric_limits<uid_t>::max())return absent<std::string>("UID outside native range");
    std::vector<char> buffer(1024);passwd entry{},*found=nullptr;
    for(;;){const int rc=::getpwuid_r(static_cast<uid_t>(uid),&entry,buffer.data(),buffer.size(),&found);if(rc==ERANGE&&buffer.size()<1024*1024){buffer.resize(buffer.size()*2);continue;}if(rc){const auto f=failure(rc);return {{},f.support,"NSS lookup: "+f.note};}if(!found||!entry.pw_name)return absent<std::string>("UID has no NSS account name");return {display(entry.pw_name),Support::estimated,"Current caller NSS real-UID name; not a persistent identity, lookup may block"};}
#else
    (void)uid;return absent<std::string>("Linux-only NSS provider");
#endif
}
Metric<std::string> linux_executable_architecture(std::span<const unsigned char> h){
    const auto invalid=[](){return absent<std::string>("Incomplete/unsupported ELF executable header; no architecture inferred from host");};
    if(h.size()<52||h[0]!=0x7f||h[1]!='E'||h[2]!='L'||h[3]!='F'||(h[4]!=1&&h[4]!=2)||(h[5]!=1&&h[5]!=2)||h[6]!=1)return invalid();
    const std::size_t size=h[4]==1?52:64;if(h.size()<size)return invalid();
    auto word=[&](std::size_t at){return h[5]==1?unsigned(h[at])|(unsigned(h[at+1])<<8):(unsigned(h[at])<<8)|unsigned(h[at+1]);};
    auto dword=[&](std::size_t at){return h[5]==1?word(at)|(word(at+2)<<16):(word(at)<<16)|word(at+2);};
    if((word(16)!=2&&word(16)!=3)||dword(20)!=1||word(h[4]==1?40:52)!=size)return invalid();
    const auto architecture=linux_elf_architecture(h);if(architecture.empty())return invalid();
    return {architecture,Support::available,"ELF class/machine of pinned executable header, not OS/CPU architecture or a full ELF integrity check"};
}
bool linux_executable_matches(const LinuxProcessStat& expected,const std::optional<LinuxProcessStat>& before,const std::optional<LinuxProcessStat>& after,const LinuxExecutableStamp& first,const LinuxExecutableStamp& last,std::string_view first_path,std::string_view last_path){
    auto same=[&](const auto& row){return row&&row->pid==expected.pid&&row->start_ticks==expected.start_ticks;};
    return expected.pid&&same(before)&&same(after)&&first==last&&!first_path.empty()&&first_path==last_path&&first_path.size()<=65536&&first_path.find('\0')==first_path.npos;
}
Metric<LinuxExecutableInfo> read_linux_process_executable(const std::filesystem::path& path,const LinuxProcessStat& expected){
    if(!expected.pid)return absent<LinuxExecutableInfo>("Invalid expected PID");
#ifdef __linux__
    auto error=[](int code){const auto f=failure(code);return Metric<LinuxExecutableInfo>{{},f.support,f.note};};
    const Fd dir{::open(path.c_str(),O_PATH|O_DIRECTORY|O_CLOEXEC)};if(dir.value<0){const int e=errno;return error(e);}
    const auto first_stat=read_at(dir.value,"stat",65536);if(!first_stat.value)return {{},first_stat.support,first_stat.note};
    const auto before=parse_linux_process_stat(*first_stat.value);
    if(!before||before->pid!=expected.pid||before->start_ticks!=expected.start_ticks)return absent<LinuxExecutableInfo>("Process identity differs before executable read");
    // proc/PID/exe is a kernel magic link: following it with O_PATH pins the
    // target without opening devices/FIFOs for I/O. Only regular files proceed.
    const Fd target{::openat(dir.value,"exe",O_PATH|O_CLOEXEC)};if(target.value<0){const int e=errno;return error(e);}
    struct stat initial{},final{},current{};
    if(::fstat(target.value,&initial)<0){const int e=errno;return error(e);}
    if(!S_ISREG(initial.st_mode)||initial.st_size<0)return absent<LinuxExecutableInfo>("Executable target is not a regular file");
    const auto first_path=exe_link(dir.value);if(!first_path.value)return {{},first_path.support,first_path.note};
    const auto architecture=architecture_from_fd(target.value,stamp(initial));
    const auto last_path=exe_link(dir.value);if(!last_path.value)return {{},last_path.support,last_path.note};
    const Fd last_target{::openat(dir.value,"exe",O_PATH|O_CLOEXEC)};if(last_target.value<0){const int e=errno;return error(e);}
    if(::fstat(last_target.value,&current)<0||::fstat(target.value,&final)<0){const int e=errno;return error(e);}
    const auto last_stat=read_at(dir.value,"stat",65536);if(!last_stat.value)return {{},last_stat.support,last_stat.note};
    if(stamp(initial)!=stamp(final)||!linux_executable_matches(expected,before,parse_linux_process_stat(*last_stat.value),stamp(initial),stamp(current),*first_path.value,*last_path.value))return absent<LinuxExecutableInfo>("Executable/process identity, metadata or link changed; observation discarded");
    return {LinuxExecutableInfo{first_path,architecture,stamp(initial)},Support::estimated,"Pinned regular executable; before/after identity checks are not atomic and cannot detect all exec/ABA changes"};
#else
    (void)path;return absent<LinuxExecutableInfo>("Linux-only executable provider");
#endif
}
void apply_linux_process_details(ProcessInfo& p,const Metric<LinuxProcessStatus>& status,const Metric<std::string>& account,const Metric<LinuxExecutableInfo>& exe,const Metric<LinuxProcessDescription>& description){
    auto text_failure=[](Support support,const std::string& note){return Metric<std::string>{{},support==Support::permission_required?Support::permission_required:Support::unsupported,note};};
    LinuxProcessDetails d;
    d.name=d.status=text_failure(status.support,status.note);
    d.real_uid={{},d.name.support,status.note};
    if(status.value){
        if(status.value->name)d.name={status.value->name,Support::estimated,status.note};
        if(status.value->state)d.status={status.value->state,Support::estimated,status.note};
        if(status.value->real_uid)d.real_uid={status.value->real_uid,Support::estimated,status.note};
    }
    d.username=d.real_uid.value?account:text_failure(d.real_uid.support,"Real UID unavailable; no account attribution");
    if(exe.value){d.executable=exe.value->path;d.architecture=exe.value->architecture;}
    else d.executable=d.architecture=text_failure(exe.support,exe.note);
    if(description.value){d.description={description.value->text,Support::estimated,description.note};d.description_source={description.value->source,Support::estimated,description.note};}
    else d.description=d.description_source=text_failure(description.support,description.note);
    p.name=d.name.value.value_or("");p.status=d.status.value.value_or("");p.username=d.username.value.value_or("");
    p.executable=d.executable.value.value_or("");p.architecture=d.architecture.value.value_or("");p.description=d.description.value.value_or("");p.linux_details=std::move(d);
}
}
