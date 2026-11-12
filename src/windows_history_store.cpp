#include "pcinsight/windows_history_store.hpp"
#include "history_file.hpp"
#include <array>
#include <fstream>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <type_traits>
namespace pcinsight {
namespace {
constexpr std::array<char,8> magic{'P','C','I','W','H','S','T',0};
constexpr std::size_t max_file=64*1024*1024,max_string=1048576,max_entries=100000;
using Clock=std::chrono::system_clock;using Ms=std::chrono::milliseconds;
std::int64_t ms(Clock::time_point t){return std::chrono::duration_cast<Ms>(t.time_since_epoch()).count();}
std::uint64_t checksum(std::string_view bytes){std::uint64_t h=14695981039346656037ULL;for(unsigned char c:bytes){h^=c;h*=1099511628211ULL;}return h;}
void put(std::ostream& o,std::uint64_t value,unsigned count){for(unsigned i=0;i<count;++i)o.put(static_cast<char>((value>>(i*8))&255));}
bool get(std::istream& in,std::uint64_t& value,unsigned count){value=0;for(unsigned i=0;i<count;++i){const auto c=in.get();if(c==std::char_traits<char>::eof())return false;value|=std::uint64_t(static_cast<unsigned char>(c))<<(i*8);}return true;}
void string_out(std::ostream& out,const std::string& text){put(out,text.size(),4);out.write(text.data(),static_cast<std::streamsize>(text.size()));}
bool string_in(std::istream& in,std::string& text){std::uint64_t size=0;if(!get(in,size,4)||size>max_string)return false;text.resize(static_cast<std::size_t>(size));return static_cast<bool>(in.read(text.data(),static_cast<std::streamsize>(size)));}
bool known(Support support){switch(support){case Support::available:case Support::estimated:return true;case Support::permission_required:case Support::unsupported:return false;}throw std::invalid_argument("invalid history metric support");}
template<class T> bool valid_metric(const Metric<T>& m){return known(m.support)==m.value.has_value() && m.note.size()<=max_string;}
bool valid(const WindowsAppHistoryEntry& e){
    return !e.id.empty() && e.id.size()<=max_string && e.name.size()<=max_string && e.id.find('\0')==e.id.npos && e.name.find('\0')==e.name.npos &&
        e.period_start.time_since_epoch().count()>=0 && e.period_end>=e.period_start && valid_metric(e.observed_cpu_time) && valid_metric(e.network_bytes) && valid_metric(e.notifications) &&
        (!e.observed_cpu_time.value || e.observed_cpu_time.value->count()>=0);
}
template<class T> std::size_t metric_size(const Metric<T>& m){return 6+m.note.size()+(m.value?8:0);}
std::size_t entry_size(const WindowsAppHistoryEntry& e){return 24+e.id.size()+e.name.size()+metric_size(e.observed_cpu_time)+metric_size(e.network_bytes)+metric_size(e.notifications);}
void validate(std::span<const WindowsAppHistoryEntry> entries){
    if(entries.size()>max_entries)throw std::invalid_argument("too many Windows history entries");
    std::size_t size=24;for(const auto& row:entries){if(!valid(row))throw std::invalid_argument("invalid Windows history entry/metric");const auto cost=entry_size(row);if(cost>max_file-size)throw std::invalid_argument("Windows history exceeds 64 MiB");size+=cost;}
}
template<class T> void metric_out(std::ostream& out,const Metric<T>& m){
    put(out,static_cast<unsigned>(m.support),1);put(out,m.value?1:0,1);
    if(m.value){if constexpr(std::is_same_v<T,std::chrono::nanoseconds>)put(out,static_cast<std::uint64_t>(m.value->count()),8);else put(out,*m.value,8);}
    string_out(out,m.note);
}
template<class T> bool metric_in(std::istream& in,Metric<T>& m){
    std::uint64_t support=0,present=0,value=0;if(!get(in,support,1)||support>3||!get(in,present,1)||present>1)return false;
    m.support=static_cast<Support>(support);if(known(m.support)!=(present==1))return false;
    if(present){if(!get(in,value,8))return false;if constexpr(std::is_same_v<T,std::chrono::nanoseconds>){if(value>static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))return false;m.value=std::chrono::nanoseconds(static_cast<std::int64_t>(value));}else m.value=value;}
    return string_in(in,m.note);
}
template<class T> bool decreased(const Metric<T>& before,const Metric<T>& after){return before.value && (!after.value || *after.value<*before.value);}
}
WindowsHistoryLoadResult WindowsHistoryStore::load(const std::filesystem::path& path){
    auto fail=[](const char* note){WindowsHistoryLoadResult r;r.error=note;return r;};
    if(path.empty())return fail("empty Windows history path");
    std::ifstream file(path,std::ios::binary);if(!file)return fail("cannot open Windows history");
    std::string bytes;std::array<char,4096> chunk{};
    while(file){file.read(chunk.data(),chunk.size());const auto count=static_cast<std::size_t>(file.gcount());if(count>max_file-bytes.size())return fail("Windows history exceeds 64 MiB");bytes.append(chunk.data(),count);}
    if(!file.eof())return fail("Windows history read failed");
    if(bytes.size()<24)return fail("truncated Windows history");
    std::istringstream trailer(bytes.substr(bytes.size()-8),std::ios::binary);std::uint64_t stored=0;
    if(!get(trailer,stored,8)||stored!=checksum(std::string_view(bytes).substr(0,bytes.size()-8)))return fail("Windows history checksum mismatch");
    bytes.resize(bytes.size()-8);std::istringstream in(bytes,std::ios::binary);std::array<char,8> header{};std::uint64_t version=0,count=0;
    if(!in.read(header.data(),8)||header!=magic)return fail("not a Windows history file (legacy format not converted)");
    if(!get(in,version,4)||version!=1)return fail("unsupported Windows history version");
    if(!get(in,count,4)||count>max_entries||count>(bytes.size()-16)/43)return fail("invalid Windows history row count");
    WindowsHistoryLoadResult result;
    for(std::uint64_t i=0;i<count;++i){
        WindowsAppHistoryEntry row;std::uint64_t start=0,end=0;
        if(!string_in(in,row.id)||!string_in(in,row.name)||!get(in,start,8)||!get(in,end,8)||start>end||end>static_cast<std::uint64_t>(ms(Clock::time_point::max())) ||
            !metric_in(in,row.observed_cpu_time)||!metric_in(in,row.network_bytes)||!metric_in(in,row.notifications))return fail("invalid/truncated Windows history record");
        row.period_start=Clock::time_point(Ms(start));row.period_end=Clock::time_point(Ms(end));if(!valid(row))return fail("invalid Windows history entry");
        result.entries.push_back(std::move(row));
    }
    if(in.peek()!=std::char_traits<char>::eof())return fail("trailing Windows history bytes");
    result.success=true;result.format_version=1;return result;
}
HistoryIoResult WindowsHistoryStore::save(const std::filesystem::path& path,std::span<const WindowsAppHistoryEntry> entries){
    if(path.empty())return {false,"empty Windows history path"};
    try{validate(entries);}catch(const std::invalid_argument& e){return {false,e.what()};}
    std::error_code error;const auto exists=std::filesystem::exists(path,error);
    if(error)return {false,"cannot inspect Windows history destination: "+error.message()};
    if(exists){const auto previous=load(path);if(!previous)return {false,"existing Windows history is invalid or a different format; refusing replacement: "+previous.error};}
    std::ostringstream out(std::ios::binary);out.write(magic.data(),8);put(out,1,4);put(out,entries.size(),4);
    for(const auto& row:entries){string_out(out,row.id);string_out(out,row.name);put(out,ms(row.period_start),8);put(out,ms(row.period_end),8);metric_out(out,row.observed_cpu_time);metric_out(out,row.network_bytes);metric_out(out,row.notifications);}
    if(!out)return {false,"Windows history encoding failed"};
    const auto body=out.str();put(out,checksum(body),8);if(!out)return {false,"Windows history checksum encoding failed"};return detail::replace_history_file(path,out.str());
}
std::vector<WindowsAppHistoryEntry> WindowsHistoryStore::merge(std::span<const WindowsAppHistoryEntry> persisted,std::span<const WindowsAppHistoryEntry> current){
    validate(persisted);validate(current);std::map<std::pair<std::string,std::int64_t>,WindowsAppHistoryEntry> rows;
    auto ingest=[&](const WindowsAppHistoryEntry& entry){
        auto [it,inserted]=rows.emplace(std::make_pair(entry.id,ms(entry.period_start)),entry);
        if(rows.size()>max_entries)throw std::invalid_argument("merged Windows history exceeds row limit");
        if(!inserted && ms(entry.period_end)>=ms(it->second.period_end)){
            const auto& previous=it->second;
            if(decreased(previous.observed_cpu_time,entry.observed_cpu_time)||decreased(previous.network_bytes,entry.network_bytes)||decreased(previous.notifications,entry.notifications))throw std::invalid_argument("Windows cumulative history decreased/lost known metric; start a new period");
            it->second=entry;
        }
    };
    for(const auto& e:persisted)ingest(e);
    for(const auto& e:current)ingest(e);
    std::vector<WindowsAppHistoryEntry> out;for(auto& [key,row]:rows)out.push_back(std::move(row));validate(out);return out;
}
std::vector<WindowsAppHistoryEntry> WindowsHistoryStore::retain_since(std::span<const WindowsAppHistoryEntry> entries,Clock::time_point cutoff){
    validate(entries);std::vector<WindowsAppHistoryEntry> out;for(const auto& row:entries)if(ms(row.period_end)>=ms(cutoff))out.push_back(row);return out;
}
HistoryIoResult WindowsHistoryStore::prune(const std::filesystem::path& path,Clock::time_point cutoff){const auto read=load(path);if(!read)return {false,read.error};return save(path,retain_since(read.entries,cutoff));}
HistoryIoResult WindowsHistoryStore::reset(const std::filesystem::path& path){if(path.empty())return {false,"empty Windows history path"};return HistoryStore::reset(path);}
}
