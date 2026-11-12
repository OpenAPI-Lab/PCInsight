#include "linux_process_metadata.hpp"
#include "cpu_identity_text.hpp"
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdlib>
#include <map>
#include <set>
#ifdef __linux__
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
namespace pcinsight::detail {
namespace {
std::string_view trim(std::string_view s){const auto a=s.find_first_not_of(" \t\r"),b=s.find_last_not_of(" \t\r");return a==s.npos?std::string_view{}:s.substr(a,b-a+1);}
template<class T> Metric<T> absent(std::string reason){return {{},Support::unsupported,std::move(reason)};}
std::optional<std::string> program(std::string_view exec){
    // Conservative subset: simple program plus standalone file/URL field codes.
    // Literal arguments/wrappers are not treated as unique application identity.
    if(exec.empty()||exec.size()>4096||exec.find('\0')!=exec.npos)return {};
    std::string first;
    if(exec.front()=='"'){const auto end=exec.find('"',1);if(end==exec.npos)return {};first=exec.substr(1,end-1);exec.remove_prefix(end+1);if(!exec.empty()&&exec.front()!=' ')return {};}
    else{const auto end=exec.find(' ');first=exec.substr(0,end);exec=end==exec.npos?std::string_view{}:exec.substr(end);if(first.find_first_of("'\"<>~|&;$*?#()`)\\")!=first.npos)return {};}
    if(first.empty()||first.find_first_of("\\$`=%\n\r\t")!=first.npos)return {};
    for(unsigned char c:first)if(c<32||c>=127)return {};
    if(first.front()!='/'&&first.find('/')!=first.npos)return {};
    unsigned file_codes=0;
    while(!(exec=trim(exec)).empty()){const auto end=exec.find(' ');const auto arg=exec.substr(0,end);if(arg!="%f"&&arg!="%F"&&arg!="%u"&&arg!="%U")return {};if(++file_codes>1)return {};exec=end==exec.npos?std::string_view{}:exec.substr(end);}
    return first;
}
std::optional<std::string> comment(std::string_view text){
    if(text.size()>1024)return {};
    std::string decoded;
    for(std::size_t n=0;n<text.size();++n){char c=text[n];if(c=='\\'){if(++n==text.size())return {};c=text[n];if(c=='s')c=' ';else if(c=='t')c='\t';else if(c!='\\')return {};}decoded+=c;}
    return cpu_identity_text(decoded);
}
#ifdef __linux__
struct Fd{int value;~Fd(){if(value>=0)::close(value);}};
LinuxExecutableStamp stamp(const struct stat& s){return {static_cast<std::uint64_t>(s.st_dev),static_cast<std::uint64_t>(s.st_ino),static_cast<std::uint64_t>(s.st_size),static_cast<std::uint64_t>(s.st_mode),s.st_mtim.tv_sec,s.st_mtim.tv_nsec,s.st_ctim.tv_sec,s.st_ctim.tv_nsec};}
std::optional<LinuxExecutableStamp> file_stamp(const std::filesystem::path& path){
    const Fd fd{::open(path.c_str(),O_PATH|O_CLOEXEC)};struct stat st{};if(fd.value<0||::fstat(fd.value,&st)||!S_ISREG(st.st_mode)||st.st_size<0)return {};return stamp(st);
}
std::optional<std::pair<std::string,LinuxExecutableStamp>> metadata(const std::filesystem::path& path){
    const Fd pin{::open(path.c_str(),O_PATH|O_CLOEXEC)};struct stat before{},after{};
    if(pin.value<0||::fstat(pin.value,&before)||!S_ISREG(before.st_mode)||before.st_size<0||before.st_size>65536)return {};
    const auto fd_path="/proc/self/fd/"+std::to_string(pin.value);const Fd fd{::open(fd_path.c_str(),O_RDONLY|O_CLOEXEC|O_NONBLOCK)};
    if(fd.value<0||::fstat(fd.value,&after)||stamp(before)!=stamp(after))return {};
    std::string text;std::array<char,4096> buffer{};unsigned interrupts=0;
    for(;;){const auto n=::read(fd.value,buffer.data(),buffer.size());if(n<0){if(errno==EINTR&&++interrupts<=3)continue;return {};}if(!n)break;if(static_cast<std::size_t>(n)>65536-text.size())return {};text.append(buffer.data(),static_cast<std::size_t>(n));}
    if(::fstat(fd.value,&after)||stamp(before)!=stamp(after))return {};
    return std::pair{std::move(text),stamp(after)};
}
std::optional<LinuxExecutableStamp> resolve_program(const std::string& name,std::string_view path){
    auto executable=[](const auto& s){return s&&(s->mode&0111);};
    if(name.front()=='/'){const auto s=file_stamp(name);return executable(s)?s:std::nullopt;}
    unsigned count=0;while(!path.empty()){if(++count>128)return {};const auto end=path.find(':');const auto part=path.substr(0,end);if(part.empty()||part.front()!='/')return {};const auto s=file_stamp(std::filesystem::path(part)/name);if(executable(s))return s;if(end==path.npos)break;path.remove_prefix(end+1);}return {};
}
#endif
}
std::optional<LinuxDesktopDescription> parse_linux_desktop_description(std::string_view text){
    if(text.empty()||text.size()>65536||text.find('\0')!=text.npos)return {};
    bool main=false,seen=false;std::map<std::string,std::string> keys;unsigned lines=0;
    while(!text.empty()){
        const auto end=text.find('\n');const auto line=trim(text.substr(0,end));if(++lines>4096||line.size()>8192)return {};
        if(!line.empty()&&line.front()!='#'){
            if(line.front()=='['){if(line.back()!=']')return {};main=line=="[Desktop Entry]";if(main){if(seen)return {};seen=true;}}
            else if(!seen)return {};
            else if(main){const auto equal=line.find('=');if(equal==line.npos)return {};const auto key=trim(line.substr(0,equal));if(key=="Type"||key=="Name"||key=="Exec"||key=="Comment"||key=="Hidden"||key=="DBusActivatable"){if(!keys.emplace(std::string(key),std::string(trim(line.substr(equal+1)))).second)return {};}}
        }
        if(end==text.npos)break;
        text.remove_prefix(end+1);
    }
    if(keys["Type"]!="Application"||!cpu_identity_text(keys["Name"]))return {};
    for(const auto* flag:{"Hidden","DBusActivatable"})if(keys.contains(flag)&&keys[flag]!="false")return {};
    const auto exe=program(keys["Exec"]),description=comment(keys["Comment"]);if(!exe||!description)return {};
    return LinuxDesktopDescription{*exe,*description};
}
LinuxDescriptionIndex read_linux_description_index(const std::vector<std::filesystem::path>& roots,std::string_view search_path){
    if(roots.size()>64||search_path.size()>32768||search_path.find('\0')!=search_path.npos)return absent<std::vector<LinuxDescriptionCandidate>>("Description roots/PATH exceed bounds");
#ifdef __linux__
    std::map<std::string,std::filesystem::path> selected;std::size_t visited=0,payload=0;std::vector<LinuxDescriptionCandidate> out;
    for(const auto& root:roots){
        if(!root.is_absolute()||root.string().size()>32768||root.string().find('\0')!=std::string::npos)return absent<std::vector<LinuxDescriptionCandidate>>("Description root must be bounded absolute path");
        std::error_code ec;std::filesystem::recursive_directory_iterator it(root,ec),end;
        if(ec==std::errc::no_such_file_or_directory)continue;
        if(ec)return {{},ec==std::errc::permission_denied?Support::permission_required:Support::unsupported,"Description catalogue enumeration failed; no partial associations"};
        std::map<std::string,std::filesystem::path> local;
        for(;it!=end&&!ec;it.increment(ec)){
            if(++visited>16384||it.depth()>8)return absent<std::vector<LinuxDescriptionCandidate>>("Description enumeration bound exceeded");
            if(it->path().extension()!=".desktop")continue;
            auto id=it->path().lexically_relative(root).generic_string();std::replace(id.begin(),id.end(),'/','-');
            if(local.contains(id))local[id].clear();else local.emplace(id,it->path());
            if(local.size()>4096)return absent<std::vector<LinuxDescriptionCandidate>>("Description file bound exceeded");
        }
        if(ec)return absent<std::vector<LinuxDescriptionCandidate>>("Description catalogue enumeration incomplete");
        for(const auto& [id,path]:local)selected.try_emplace(id,path); // higher root shadows even hidden/malformed/unreadable
        if(selected.size()>4096)return absent<std::vector<LinuxDescriptionCandidate>>("Description catalogue bound exceeded");
    }
    for(const auto& [id,path]:selected){
        (void)id;if(path.empty())continue;const auto raw=metadata(path);if(!raw)continue;
        if(raw->first.size()>16*1024*1024-payload)return absent<std::vector<LinuxDescriptionCandidate>>("Description payload bound exceeded");
        payload+=raw->first.size();
        const auto entry=parse_linux_desktop_description(raw->first);if(!entry)continue;const auto exe=resolve_program(entry->program,search_path);if(!exe)continue;
        out.push_back({*exe,raw->second,entry->comment,path.string()});
    }
    return {std::move(out),Support::estimated,"Caller XDG catalogue; unlocalized Comment, conservative Exec subset; association by file stamp, not proof of launch or publisher"};
#else
    (void)roots;return absent<std::vector<LinuxDescriptionCandidate>>("Linux-only metadata catalogue");
#endif
}
LinuxDescriptionIndex read_native_linux_description_index(){
#ifdef __linux__
    auto env=[](const char* key)->std::optional<std::string>{const auto* p=std::getenv(key);if(!p)return std::string{};for(std::size_t n=0;n<=32768;++n)if(!p[n])return std::string(p,n);return {};};
    const auto h=env("HOME"),d=env("XDG_DATA_HOME"),ds=env("XDG_DATA_DIRS"),ps=env("PATH");
    if(!h||!d||!ds||!ps)return absent<std::vector<LinuxDescriptionCandidate>>("XDG environment exceeds bounds");
    const auto& home=*h;const auto& data=*d;const auto& dirs=*ds;const auto& path=*ps;
    std::vector<std::filesystem::path> roots;std::set<std::filesystem::path> seen;
    auto add=[&](const std::string& root){if(!root.empty()&&root.front()=='/'){auto p=(std::filesystem::path(root)/"applications").lexically_normal();if(seen.insert(p).second)roots.push_back(std::move(p));}};
    if(!data.empty()&&data.front()=='/')add(data);else if(!home.empty()&&home.front()=='/')add(home+"/.local/share");
    std::string_view system=dirs.empty()?std::string_view("/usr/local/share:/usr/share"):std::string_view(dirs);
    while(!system.empty()){const auto end=system.find(':');add(std::string(system.substr(0,end)));if(end==system.npos)break;system.remove_prefix(end+1);}
    return read_linux_description_index(roots,path);
#else
    return absent<std::vector<LinuxDescriptionCandidate>>("Linux-only metadata catalogue");
#endif
}
Metric<LinuxProcessDescription> lookup_linux_description(const LinuxDescriptionIndex& index,const Metric<LinuxExecutableInfo>& exe){
    if(!exe.value)return {{},exe.support,exe.note};
    if(!index.value)return {{},index.support,index.note};
    const LinuxDescriptionCandidate* match=nullptr;
    for(const auto& row:*index.value)if(row.executable==exe.value->stamp){if(match)return absent<LinuxProcessDescription>("Multiple desktop associations; description ambiguous");match=&row;}
    if(!match)return absent<LinuxProcessDescription>("No supported matching desktop Comment; no description inferred from process name");
#ifdef __linux__
    if(file_stamp(match->source)!=std::optional{match->metadata})return absent<LinuxProcessDescription>("Description source changed/disappeared after catalogue read");
#endif
    return {LinuxProcessDescription{match->comment,match->source},Support::estimated,index.note};
}
}
