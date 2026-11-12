#include "pcinsight/history_store.hpp"
#include "history_file.hpp"
#include <array>
#include <chrono>
#include <fstream>
#include <map>
#include <limits>
#include <sstream>
#include <stdexcept>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <cerrno>
#include <unistd.h>
#endif

namespace pcinsight {
namespace {
constexpr std::array<char,8> magic{'P','C','I','H','I','S','T',0};
constexpr std::size_t max_file=64*1024*1024, max_string=1048576, max_entries=100000;
using Clock=std::chrono::system_clock;
using Ms=std::chrono::milliseconds;
// FNV-1a detects accidental corruption; it is not an authentication mechanism.
std::uint64_t checksum(std::string_view bytes){
    std::uint64_t hash=14695981039346656037ULL;
    for(unsigned char c:bytes){hash^=c;hash*=1099511628211ULL;}
    return hash;
}
std::int64_t ms(Clock::time_point t){return std::chrono::duration_cast<Ms>(t.time_since_epoch()).count();}
bool valid(const AppHistoryEntry& e){
    return !e.id.empty() && e.id.size()<=max_string && e.name.size()<=max_string &&
        e.cpu_time.count()>=0 && ms(e.period_start)>=0 && e.period_end>=e.period_start;
}
void put(std::ostream& o,std::uint64_t v,unsigned n){for(unsigned i=0;i<n;++i)o.put(static_cast<char>((v>>(8*i))&255));}
bool get(std::istream& in,std::uint64_t& v,unsigned n){
    v=0;for(unsigned i=0;i<n;++i){auto c=in.get();if(c==std::char_traits<char>::eof())return false;v|=static_cast<std::uint64_t>(static_cast<unsigned char>(c))<<(8*i);}return true;
}
void putstr(std::ostream& o,const std::string& s){put(o,s.size(),4);o.write(s.data(),static_cast<std::streamsize>(s.size()));}
bool getstr(std::istream& in,std::string& s){
    std::uint64_t n=0;if(!get(in,n,4)||n>max_string)return false;
    s.resize(static_cast<std::size_t>(n));return static_cast<bool>(in.read(s.data(),static_cast<std::streamsize>(n)));
}
HistoryIoResult replace(const std::filesystem::path& path,const std::string& bytes){
#ifdef _WIN32
    HANDLE file=INVALID_HANDLE_VALUE;std::filesystem::path temp;
    const auto stamp=std::chrono::steady_clock::now().time_since_epoch().count();
    for(unsigned attempt=0;attempt<100;++attempt){
        temp=path;temp+=L".tmp-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(stamp)+L"-"+std::to_wstring(attempt);
        file=CreateFileW(temp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(file!=INVALID_HANDLE_VALUE)break;
        if(GetLastError()!=ERROR_FILE_EXISTS)return {false,"cannot create temporary history file"};
    }
    if(file==INVALID_HANDLE_VALUE)return {false,"temporary name collision"};
    DWORD written=0;
    bool ok=WriteFile(file,bytes.data(),static_cast<DWORD>(bytes.size()),&written,nullptr)&&written==bytes.size();
    if(ok)ok=FlushFileBuffers(file)!=0;
    if(!CloseHandle(file))ok=false;
    if(ok)ok=MoveFileExW(temp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
    if(!ok){auto err=GetLastError();DeleteFileW(temp.c_str());return {false,"history replacement failed: "+std::to_string(err)};}
#else
    std::string temp=path.string()+".tmp-XXXXXX";
    int fd=mkstemp(temp.data());if(fd<0)return {false,std::error_code(errno,std::generic_category()).message()};
    std::size_t offset=0;int error=0;
    while(offset<bytes.size()){
        auto n=write(fd,bytes.data()+offset,bytes.size()-offset);
        if(n<0&&errno==EINTR)continue;
        if(n<=0){error=n<0?errno:EIO;break;}offset+=static_cast<std::size_t>(n);
    }
    if(!error&&fsync(fd)!=0)error=errno;
    if(close(fd)!=0&&!error)error=errno;
    if(!error&&rename(temp.c_str(),path.c_str())!=0)error=errno;
    if(error){unlink(temp.c_str());return {false,std::error_code(error,std::generic_category()).message()};}
#endif
    return {true,{}};
}
}
namespace detail {HistoryIoResult replace_history_file(const std::filesystem::path& path,const std::string& bytes){return replace(path,bytes);}}
HistoryLoadResult HistoryStore::load(const std::filesystem::path& path){
    auto fail=[](const char* msg){HistoryLoadResult r;r.error=msg;return r;};
    std::ifstream file(path,std::ios::binary);if(!file)return fail("cannot open history");
    std::string bytes;std::array<char,4096> chunk{};
    while(file){
        file.read(chunk.data(),chunk.size());bytes.append(chunk.data(),static_cast<std::size_t>(file.gcount()));
        if(bytes.size()>max_file)return fail("history exceeds size limit");
    }
    if(!file.eof())return fail("history read failed");
    std::istringstream in(bytes,std::ios::binary);std::array<char,8> header{};
    if(!in.read(header.data(),header.size())||header!=magic)return fail("invalid history header");
    std::uint64_t version=0,count=0;
    if(!get(in,version,4)||(version!=1&&version!=2))return fail("unsupported history version");
    if(version==2){
        if(bytes.size()<24)return fail("truncated history checksum");
        std::istringstream trailer(bytes.substr(bytes.size()-8),std::ios::binary);
        std::uint64_t stored=0;
        if(!get(trailer,stored,8)||stored!=checksum(std::string_view(bytes).substr(0,bytes.size()-8)))
            return fail("history checksum mismatch");
        bytes.resize(bytes.size()-8);
        in.str(bytes);in.clear();in.seekg(12);
    }
    if(!get(in,count,4)||count>max_entries||count>(bytes.size()-16)/48)return fail("invalid entry count");
    HistoryLoadResult r;
    for(std::uint64_t i=0;i<count;++i){
        AppHistoryEntry e;std::uint64_t cpu=0,start=0,end=0;
        if(!getstr(in,e.id)||!getstr(in,e.name)||!get(in,cpu,8)||!get(in,e.network_bytes,8)||
           !get(in,e.notifications,8)||!get(in,start,8)||!get(in,end,8))return fail("truncated history");
        if(cpu>static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())||
           start>end||end>static_cast<std::uint64_t>(ms(Clock::time_point::max())))return fail("invalid counters or timestamps");
        e.cpu_time=std::chrono::nanoseconds(static_cast<std::int64_t>(cpu));
        e.period_start=Clock::time_point(Ms(start));e.period_end=Clock::time_point(Ms(end));
        if(!valid(e))return fail("invalid history entry");
        r.entries.push_back(std::move(e));
    }
    if(in.peek()!=std::char_traits<char>::eof())return fail("trailing history data");
    r.format_version=static_cast<std::uint32_t>(version);
    r.success=true;return r;
}
HistoryIoResult HistoryStore::save(const std::filesystem::path& path,std::span<const AppHistoryEntry> entries){
    if(entries.size()>max_entries)return {false,"too many entries"};
    std::size_t size=24;
    for(const auto& e:entries){
        if(!valid(e))return {false,"invalid history entry"};
        size+=48+e.id.size()+e.name.size();if(size>max_file)return {false,"history exceeds size limit"};
    }
    std::ostringstream out(std::ios::binary);out.write(magic.data(),magic.size());put(out,2,4);put(out,entries.size(),4);
    for(const auto& e:entries){
        putstr(out,e.id);putstr(out,e.name);put(out,e.cpu_time.count(),8);put(out,e.network_bytes,8);
        put(out,e.notifications,8);put(out,ms(e.period_start),8);put(out,ms(e.period_end),8);
    }
    const auto body=out.str();put(out,checksum(body),8);
    return replace(path,out.str());
}
std::vector<AppHistoryEntry> HistoryStore::merge(std::span<const AppHistoryEntry> persisted,std::span<const AppHistoryEntry> current){
    std::map<std::pair<std::string,std::int64_t>,AppHistoryEntry> rows;
    auto ingest=[&](const AppHistoryEntry& e){
        if(!valid(e))throw std::invalid_argument("invalid history entry");
        auto [it,inserted]=rows.emplace(std::make_pair(e.id,ms(e.period_start)),e);
        if(!inserted&&ms(e.period_end)>=ms(it->second.period_end)){
            const auto& old=it->second;
            if(e.cpu_time<old.cpu_time||e.network_bytes<old.network_bytes||e.notifications<old.notifications)
                throw std::invalid_argument("cumulative counters decreased; start a new period after reset");
            it->second=e;
        }
    };
    for(const auto& e:persisted)ingest(e);
    for(const auto& e:current)ingest(e);
    std::vector<AppHistoryEntry> out;out.reserve(rows.size());
    for(auto& [key,e]:rows)out.push_back(std::move(e));
    return out;
}
HistoryIoResult HistoryStore::reset(const std::filesystem::path& path){
    std::error_code ec;std::filesystem::remove(path,ec);
    return {!ec,ec?ec.message():std::string{}};
}
std::vector<AppHistoryEntry> HistoryStore::retain_since(
    std::span<const AppHistoryEntry> entries,Clock::time_point cutoff){
    std::vector<AppHistoryEntry> out;
    for(const auto& e:entries){
        if(!valid(e))throw std::invalid_argument("invalid history entry");
        if(ms(e.period_end)>=ms(cutoff))out.push_back(e);
    }
    return out;
}
HistoryIoResult HistoryStore::prune(const std::filesystem::path& path,Clock::time_point cutoff){
    auto loaded=load(path);if(!loaded)return {false,loaded.error};
    auto retained=retain_since(loaded.entries,cutoff);
    return save(path,retained);
}
HistoryIoResult HistoryStore::migrate(const std::filesystem::path& path){
    auto loaded=load(path);if(!loaded)return {false,loaded.error};
    if(loaded.format_version==2)return {true,{}};
    return save(path,loaded.entries);
}
}
