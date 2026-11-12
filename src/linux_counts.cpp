#include "linux_counts.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <cerrno>
#include <fstream>
#include <limits>
#include <string_view>

namespace pcinsight::detail {
namespace {
bool numeric(std::string_view name) {
    return !name.empty() && std::all_of(name.begin(), name.end(),
        [](char c) { return c >= '0' && c <= '9'; });
}
struct Count {
    std::uint64_t total{}, readings{};
    bool incomplete{}, denied{}, overflow{};
    void add(std::uint64_t n) {
        ++readings;
        if (n > std::numeric_limits<std::uint64_t>::max() - total) overflow = true;
        else total += n;
    }
    void failure(bool permission) { incomplete = true; denied |= permission; }
    Metric<std::uint64_t> metric(std::string note, bool proxy = false) const {
        if (overflow) return {std::nullopt, Support::unsupported, note + "; desbordamiento"};
        if (!readings && incomplete)
            return {std::nullopt, denied ? Support::permission_required : Support::unsupported,
                note + "; no se pudo leer ningún conteo"};
        return {total, incomplete || proxy ? Support::estimated : Support::available,
            note + (incomplete ? "; conteo parcial por errores, permisos o procesos terminados" : "")};
    }
};
bool permission(const std::error_code& ec) {
    return ec == std::errc::permission_denied || ec == std::errc::operation_not_permitted;
}
std::optional<std::uint64_t> threads(const std::filesystem::path& path, bool& denied) {
    errno = 0;
    std::ifstream file(path, std::ios::binary);
    if (!file) { denied = errno == EACCES || errno == EPERM; return {}; }
    std::array<char, 65536> buffer{};
    file.read(buffer.data(), buffer.size());
    if (!file.eof() || file.bad()) return {}; // bounded, complete status only
    std::string_view text(buffer.data(), static_cast<std::size_t>(file.gcount()));
    std::optional<std::uint64_t> result;
    while (!text.empty()) {
        const auto end = text.find('\n');
        auto line = text.substr(0, end);
        if (line.starts_with("Threads:")) {
            if (result) return {}; // duplicates are malformed
            line.remove_prefix(8);
            while (!line.empty() && (line.front() == ' ' || line.front() == '\t')) line.remove_prefix(1);
            while (!line.empty() && (line.back() == ' ' || line.back() == '\t' || line.back() == '\r')) line.remove_suffix(1);
            std::uint64_t value{};
            const auto parsed = std::from_chars(line.data(), line.data() + line.size(), value);
            if (parsed.ec != std::errc{} || parsed.ptr != line.data() + line.size() || !value) return {};
            result = value;
        }
        if (end == std::string_view::npos) break;
        text.remove_prefix(end + 1);
    }
    return result;
}
}

void read_linux_counts(CpuInfo& cpu, const std::filesystem::path& root) {
    Count processes, thread_count, descriptors;
    std::error_code ec;
    std::filesystem::directory_iterator it(root, ec), end;
    if (ec) {
        processes.failure(permission(ec));
        thread_count.failure(permission(ec));
        descriptors.failure(permission(ec));
    } else {
        processes.add(0); // a successfully enumerated empty namespace is valid zero
        std::size_t scanned = 0;
        for (; it != end && !ec; it.increment(ec)) {
            if (++scanned > 1000000) { ec = std::make_error_code(std::errc::value_too_large); break; }
            if (!numeric(it->path().filename().string())) continue;
            std::error_code type_error;
            // Do not follow fixture symlinks (procfs PID entries are directories).
            const auto type = it->symlink_status(type_error).type();
            if (type_error) {
                processes.failure(permission(type_error));
                thread_count.failure(permission(type_error));
                descriptors.failure(permission(type_error));
                continue;
            }
            if (type != std::filesystem::file_type::directory) continue;
            processes.add(1);
            bool denied = false;
            if (const auto n = threads(it->path() / "status", denied)) thread_count.add(*n);
            else thread_count.failure(denied);
            std::error_code fd_error;
            std::filesystem::directory_iterator fd(it->path() / "fd", fd_error);
            std::uint64_t count = 0;
            std::size_t visited = 0;
            for (; !fd_error && fd != end; fd.increment(fd_error)) {
                if (++visited > 1000000) { fd_error = std::make_error_code(std::errc::value_too_large); break; }
                if (numeric(fd->path().filename().string())) ++count;
            }
            if (fd_error) descriptors.failure(permission(fd_error));
            else descriptors.add(count);
        }
        if (ec) {
            processes.failure(permission(ec));
            thread_count.failure(permission(ec));
            descriptors.failure(permission(ec));
        }
        if (processes.total == 0 && !processes.incomplete) {
            thread_count.add(0);
            descriptors.add(0);
        }
    }
    cpu.processes = processes.metric("procesos visibles en procfs; namespace/hidepid pueden limitar el alcance; muestra no atómica");
    cpu.threads = thread_count.metric("suma de Threads de procesos visibles; muestra no atómica");
    cpu.handles = descriptors.metric("descriptores por proceso visible, no handles Windows ni archivos únicos; tablas compartidas pueden contarse varias veces", true);
}
}
