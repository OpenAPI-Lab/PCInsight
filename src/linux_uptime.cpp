#include "linux_uptime.hpp"
#include <array>
#include <charconv>
#include <fstream>
#include <limits>
namespace pcinsight::detail {
namespace {
using Seconds=std::chrono::seconds;
Metric<Seconds> unknown(std::string note){return {{},Support::unsupported,std::move(note)};}
std::optional<std::uint64_t> whole_seconds_field(std::string_view field){
    // Kernel proc uptime format: unsigned integer seconds + two decimal digits.
    // Parse the integer part separately so fractional digits cannot overflow it.
    if(field.size()<4||field.size()>64)return {};
    const auto dot=field.find('.');
    if(dot==field.npos||dot==0||field.size()-dot!=3)return {};
    for(auto c:field.substr(dot+1))if(c<'0'||c>'9')return {};
    std::uint64_t seconds{};
    const auto number=field.substr(0,dot);
    const auto [end,ec]=std::from_chars(number.data(),number.data()+number.size(),seconds);
    if(ec!=std::errc{}||end!=number.data()+number.size())return {};
    return seconds;
}
}
Metric<Seconds> parse_linux_uptime(std::string_view text){
    if(text.empty()||text.size()>256||text.find('\0')!=text.npos)return unknown("Empty, oversized or invalid proc uptime text");
    std::array<std::string_view,2> fields{};std::size_t count=0;
    while(!text.empty()){
        const auto first=text.find_first_not_of(" \t\r\n");if(first==text.npos)break;
        text.remove_prefix(first);const auto end=text.find_first_of(" \t\r\n");
        if(count==fields.size())return unknown("Expected exactly two proc uptime fields");
        fields[count++]=text.substr(0,end);
        if(end==text.npos)break;
        text.remove_prefix(end);
    }
    if(count!=fields.size())return unknown("Expected exactly two proc uptime fields");
    const auto uptime=whole_seconds_field(fields[0]),idle=whole_seconds_field(fields[1]);
    if(!uptime||!idle)return unknown("Malformed proc uptime decimal fields; expected nonnegative seconds with two decimals");
    if(*uptime>static_cast<std::uintmax_t>(std::numeric_limits<Seconds::rep>::max()))return unknown("Proc uptime exceeds chrono::seconds representation");
    // Idle is cumulative across CPUs and may legitimately exceed uptime.
    return {Seconds(static_cast<Seconds::rep>(*uptime)),Support::available,"/proc/uptime first field, whole seconds (fraction discarded, not rounded); kernel boottime view includes suspend/time namespace offsets, not process lifetime or physical host certification"};
}
Metric<Seconds> read_linux_uptime(const std::filesystem::path& path){
#ifdef __linux__
    std::ifstream in(path,std::ios::binary);if(!in)return unknown("Proc uptime source unavailable or unreadable");
    std::array<char,257> buffer{};in.read(buffer.data(),static_cast<std::streamsize>(buffer.size()));
    if(in.bad()||in.gcount()>256)return unknown("Proc uptime source read failed or exceeded 256 bytes");
    return parse_linux_uptime(std::string_view(buffer.data(),static_cast<std::size_t>(in.gcount())));
#else
    (void)path;return unknown("Native proc uptime reader is Linux-only");
#endif
}
void fill_linux_uptime(CpuInfo& cpu,const std::filesystem::path& path){cpu.uptime=read_linux_uptime(path);}
}
