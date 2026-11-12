#include "linux_startup_inventory.hpp"
#include "linux_startup.hpp"
#include <cstdlib>
#include <fstream>
#include <map>
#include <set>
#include <stdexcept>
#ifdef __linux__
#include <cerrno>
#include <fcntl.h>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
namespace pcinsight::detail {
namespace {
constexpr std::size_t max_file = 1048576;
bool absolute(std::string_view value) { return !value.empty() && value.front() == '/' && value.find('\0') == value.npos; }
std::optional<std::string> contents(const std::filesystem::path& path) {
    std::string text; char buffer[4096];
#ifdef __linux__
    const int fd = open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NONBLOCK);
    if (fd < 0) return {};
    struct Close { int fd; ~Close() { close(fd); } } cleanup{fd};
    struct stat type{};
    if (fstat(fd, &type) != 0 || !S_ISREG(type.st_mode)) return {};
    for (;;) {
        auto n = read(fd, buffer, sizeof(buffer));
        if (n < 0) { if (errno == EINTR) continue; return {}; }
        if (!n) break;
        text.append(buffer, static_cast<std::size_t>(n));
        if (text.size() > max_file) return {};
    }
#else
    std::ifstream file(path, std::ios::binary);
    if (!file) return {};
    while (file && text.size() <= max_file) {
        file.read(buffer, sizeof(buffer)); text.append(buffer, static_cast<std::size_t>(file.gcount()));
    }
    if (!file.eof() || text.size() > max_file) return {};
#endif
    return text;
}
}
std::vector<XdgAutostartDirectory> resolve_xdg_autostart(
    std::string_view home, std::string_view config_home, std::string_view config_dirs) {
    if (home.size() > 32768 || config_home.size() > 32768 || config_dirs.size() > 1048576)
        throw std::invalid_argument("XDG directory environment exceeds limit");
    std::vector<XdgAutostartDirectory> result;
    std::set<std::filesystem::path> seen;
    auto add = [&](std::string_view path, bool personal) {
        if (!absolute(path)) return;
        if (path.size() > 32768) throw std::invalid_argument("XDG directory path exceeds limit");
        auto directory = (std::filesystem::path(path) / "autostart").lexically_normal();
        if (!seen.insert(directory).second) return;
        if (result.size() == 64) throw std::invalid_argument("too many XDG config directories");
        result.push_back({std::move(directory), personal});
    };
    if (absolute(config_home)) add(config_home, true);
    else if (absolute(home)) add((std::filesystem::path(home) / ".config").generic_string(), true);
    bool valid_system_directory = false;
    while (!config_dirs.empty()) {
        const auto end = config_dirs.find(':');
        const auto part = config_dirs.substr(0, end);
        if (absolute(part)) { add(part, false); valid_system_directory = true; }
        if (end == config_dirs.npos) break;
        config_dirs.remove_prefix(end + 1);
    }
    if (!valid_system_directory) add("/etc/xdg", false);
    return result;
}
XdgStartupInventory read_xdg_startup_inventory(const std::vector<XdgAutostartDirectory>& directories, std::size_t payload_limit,
    const XdgEligibilityContext* eligibility) {
    if (directories.size() > 64) throw std::invalid_argument("too many autostart directories");
    XdgStartupInventory result;
    std::map<std::string, StartupEntry> selected;
    bool preceding_complete = true;
    std::size_t visited = 0;
    std::size_t payload = 0;
    auto failure = [&](const std::filesystem::path& dir, const std::string& reason) {
        preceding_complete = result.complete = false;
        result.diagnostics.push_back(dir.string() + ": " + reason);
    };
    for (const auto& directory : directories) {
        std::error_code ec;
        std::filesystem::directory_iterator it(directory.path, ec), end;
        if (ec) {
            if (ec != std::errc::no_such_file_or_directory) failure(directory.path, ec.message());
            continue;
        }
        for (; it != end && !ec; it.increment(ec)) {
            if (++visited > 100000) { failure(directory.path, "entry limit reached"); break; }
            const auto path = it->path();
            if (path.extension() != ".desktop") continue;
            const auto filename = path.filename().string();
            if (selected.count(filename)) continue; // first path wins, INCLUDING Hidden=true
            std::error_code type_error;
            const bool regular = it->is_regular_file(type_error);
            if (!regular && !type_error) continue;
            StartupEntry row; row.id = path.string(); row.name = path.stem().string();
            row.source = directory.personal ? "XDG effective personal" : "XDG effective system";
            row.status = StartupStatus::unknown;
            row.xdg_standard_eligible.note = "eligibility not assessed or entry/precedence unresolved";
            row.autostart_condition_satisfied.note = "condition not assessed or entry/precedence unresolved";
            if (preceding_complete && !type_error) {
                auto text = contents(path);
                auto entry = text ? read_xdg_startup(*text) : std::nullopt;
                if (entry) {
                    if (!entry->name.empty()) row.name = std::move(entry->name);
                    row.command = std::move(entry->command); row.status = entry->status;
                    if (eligibility) {
                        row.xdg_standard_eligible = xdg_standard_eligibility(*text, *eligibility);
                        row.autostart_condition_satisfied = autostart_condition_satisfied(*text, *eligibility);
                    }
                } else row.source += "; unreadable/malformed entry";
            } else row.source += "; unresolved precedence or file type";
            const auto bytes = row.id.size() + row.name.size() + row.command.size() + row.source.size() + filename.size() + row.xdg_standard_eligible.note.size() + row.autostart_condition_satisfied.note.size();
            if (bytes > payload_limit - payload) throw std::length_error("XDG inventory metadata exceeds payload limit");
            payload += bytes;
            selected.emplace(filename, std::move(row));
        }
        if (ec) failure(directory.path, ec.message());
        if (visited > 100000) break;
    }
    for (auto& [filename, row] : selected) result.entries.push_back(std::move(row));
    return result;
}
std::vector<XdgAutostartDirectory> native_xdg_autostart_directories() {
#ifdef __linux__
    auto environment = [](const char* name) -> std::string { const auto* value = std::getenv(name); return value ? value : ""; };
    auto home = environment("HOME");
    if (!absolute(home)) if (const auto* account = getpwuid(geteuid()); account && account->pw_dir) home = account->pw_dir;
    return resolve_xdg_autostart(home, environment("XDG_CONFIG_HOME"), environment("XDG_CONFIG_DIRS"));
#else
    return {};
#endif
}
XdgStartupInventory read_native_xdg_startup_inventory() {
#ifdef __linux__
    const auto context = native_xdg_eligibility_context();
    return read_xdg_startup_inventory(native_xdg_autostart_directories(), 64 * 1024 * 1024, &context);
#else
    return {};
#endif
}
}
