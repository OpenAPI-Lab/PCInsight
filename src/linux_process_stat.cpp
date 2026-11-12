#include "linux_process_stat.hpp"
#include "linux_process_io.hpp"
#include <array>
#include <charconv>
#include <limits>
#include <sstream>
namespace pcinsight::detail {
namespace {
template<class T> bool number(std::string_view text, T& value) {
    auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    return result.ec == std::errc{} && result.ptr == text.data() + text.size();
}
}
std::optional<LinuxProcessStat> parse_linux_process_stat(std::string_view text) {
    if (text.size() > 65536 || text.find('\0')!=text.npos) return std::nullopt;
    const auto left = text.find('('), right = text.rfind(')');
    if (left == std::string_view::npos || right == std::string_view::npos || left >= right ||
        left < 2 || text[left - 1] != ' ' || right + 2 >= text.size() || text[right + 1] != ' ')
        return std::nullopt;
    LinuxProcessStat result;
    if (!number(text.substr(0,left - 1),result.pid)||!result.pid) return std::nullopt;
    auto tail = text.substr(right + 2);
    // Fields 3..22; comm may contain whitespace, newlines and parentheses.
    std::array<std::string_view,20> fields;
    for (auto& field : fields) {
        const auto begin = tail.find_first_not_of(" \t\r\n");
        if (begin == std::string_view::npos) return std::nullopt;
        tail.remove_prefix(begin);
        const auto end = tail.find_first_of(" \t\r\n");
        field = tail.substr(0,end);
        tail.remove_prefix(end == std::string_view::npos ? tail.size() : end);
    }
    std::uint64_t user{}, kernel{};
    if (fields[0].size() != 1 || !((fields[0][0]>='A'&&fields[0][0]<='Z')||(fields[0][0]>='a'&&fields[0][0]<='z')) || !number(fields[1],result.parent_pid) ||
        !number(fields[11],user) || !number(fields[12],kernel) ||
        !number(fields[17],result.threads) || !number(fields[19],result.start_ticks) ||
        kernel > std::numeric_limits<std::uint64_t>::max() - user) return std::nullopt;
    result.ticks = user + kernel;
    result.user_ticks=user;result.kernel_ticks=kernel;
    return result;
}
std::optional<std::uint64_t> linux_process_tick_delta(const LinuxProcessStat& previous,
                                                    const LinuxProcessStat& current) {
    if (previous.pid != current.pid || previous.start_ticks != current.start_ticks ||
        current.ticks < previous.ticks || current.user_ticks < previous.user_ticks ||
        current.kernel_ticks < previous.kernel_ticks) return std::nullopt;
    return current.ticks - previous.ticks;
}
std::optional<std::uint64_t> parse_linux_process_io(std::string_view text) {
    const auto counters=parse_linux_process_io_counters(text);
    if(!counters.value||counters.value->write_bytes>std::numeric_limits<std::uint64_t>::max()-counters.value->read_bytes)return {};
    return counters.value->read_bytes+counters.value->write_bytes;
}
std::string linux_elf_architecture(std::span<const unsigned char> header) {
    if (header.size() < 20 || header[0] != 0x7f || header[1] != 'E' ||
        header[2] != 'L' || header[3] != 'F' || (header[4] != 1 && header[4] != 2) ||
        (header[5] != 1 && header[5] != 2) || header[6] != 1) return {};
    const unsigned machine = header[5] == 1 ? header[18] | (unsigned(header[19])<<8)
                                           : (unsigned(header[18])<<8) | header[19];
    switch (machine) {
        case 3: return header[4] == 1 ? "x86" : "";
        case 62: return header[4] == 2 ? "x64" : "";
        case 40: return header[4] == 1 ? "ARM32" : "";
        case 183: return header[4] == 2 ? "ARM64" : "";
        case 243: return header[4] == 2 ? "RISC-V64" : "RISC-V32";
        case 20: return header[4] == 1 ? "PowerPC32" : "";
        case 21: return header[4] == 2 ? "PowerPC64" : "";
        default: return {};
    }
}
}
