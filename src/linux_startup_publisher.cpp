#include "linux_startup_publisher.hpp"
#include <algorithm>
#include <cstdlib>
#include <map>
#include <set>
#include <stdexcept>
#ifdef __linux__
#include <cerrno>
#include <dirent.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
namespace pcinsight {
namespace {
using Rows = std::vector<LinuxStartupPublisher>;
using Result = Metric<Rows>;
Result absent(const char* note, Support support = Support::unsupported) { return {std::nullopt, support, note}; }
constexpr std::size_t file_limit = 1024 * 1024, total_limit = 16 * 1024 * 1024;
bool valid_id(std::string_view id) {
    return !id.empty() && id.size() <= 4096 && id != "." && id != ".." &&
        std::all_of(id.begin(), id.end(), [](unsigned char c) { return c >= 32 && c != 127 && c != '/' && c != '\\'; });
}
bool valid_label(std::string_view value) {
    return !value.empty() && value.size() <= 4096 &&
        std::all_of(value.begin(), value.end(), [](unsigned char c) { return c >= 32 && c != 127; });
}
bool valid_component_id(std::string_view id) {
    return valid_id(id) && std::all_of(id.begin(), id.end(), [](unsigned char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '_';
    });
}
#ifdef __linux__
std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\n\r");
    if (first == value.npos) return {};
    return value.substr(first, value.find_last_not_of(" \t\n\r") - first + 1);
}
struct XmlApi {
    void* library{};
    void* (*create)(const char*, int, const char*, const char*, int){};
    void (*destroy)(void*){};
    int (*read)(void*){}, (*type)(void*){}, (*depth)(void*){}, (*empty)(void*){};
    const unsigned char* (*name)(void*){};
    const unsigned char* (*value)(void*){};
    const unsigned char* (*ns)(void*){};
    int (*attribute)(void*, const unsigned char*){}, (*element)(void*){};
    bool ready{};
    XmlApi() {
        library = dlopen("libxml2.so.2", RTLD_LOCAL | RTLD_NOW);
        if (!library) library = dlopen("libxml2.so", RTLD_LOCAL | RTLD_NOW);
        if (!library) return;
#define LOAD(member, symbol) member = reinterpret_cast<decltype(member)>(dlsym(library, symbol)); if (!member) return
        LOAD(create, "xmlReaderForMemory"); LOAD(destroy, "xmlFreeTextReader");
        LOAD(read, "xmlTextReaderRead"); LOAD(type, "xmlTextReaderNodeType");
        LOAD(depth, "xmlTextReaderDepth"); LOAD(empty, "xmlTextReaderIsEmptyElement");
        LOAD(name, "xmlTextReaderConstName"); LOAD(value, "xmlTextReaderConstValue");
        LOAD(ns, "xmlTextReaderConstNamespaceUri");
        LOAD(attribute, "xmlTextReaderMoveToAttribute"); LOAD(element, "xmlTextReaderMoveToElement");
#undef LOAD
        ready = true;
    }
    ~XmlApi() { if (library) dlclose(library); }
};
const XmlApi& xml_api() { static const XmlApi api; return api; }
bool same(const struct stat& a, const struct stat& b) {
    return a.st_dev == b.st_dev && a.st_ino == b.st_ino && a.st_mode == b.st_mode &&
        a.st_size == b.st_size && a.st_nlink == b.st_nlink &&
        a.st_mtim.tv_sec == b.st_mtim.tv_sec && a.st_mtim.tv_nsec == b.st_mtim.tv_nsec &&
        a.st_ctim.tv_sec == b.st_ctim.tv_sec && a.st_ctim.tv_nsec == b.st_ctim.tv_nsec;
}
#endif
}
namespace detail {
Result parse_linux_startup_metainfo(std::string_view text) {
    if (text.empty() || text.size() > file_limit || text.find('\0') != text.npos ||
        text.find("<!DOCTYPE") != text.npos || text.find("<!ENTITY") != text.npos)
        return absent("Missing/excessive metadata, NUL, or forbidden DTD/entity declaration");
#ifdef __linux__
    const auto& api = xml_api();
    if (!api.ready) return absent("Optional system libxml2 reader unavailable");
    // No NOENT, DTDLOAD/ATTR/VALID, XINCLUDE, RECOVER, HUGE or external URL.
    // Declarations are rejected BEFORE parsing, not just with NONET.
    void* reader = api.create(text.data(), static_cast<int>(text.size()), nullptr, "UTF-8", 32 | 64 | 2048);
    if (!reader) return absent("XML reader creation failed");
    struct Close { const XmlApi& api; void* reader; ~Close() { api.destroy(reader); } } close{api, reader};
    auto str = [](const unsigned char* value) { return value ? std::string(reinterpret_cast<const char*>(value)) : std::string{}; };
    auto attribute = [&](const char* key) -> std::optional<std::string> {
        const auto moved = api.attribute(reader, reinterpret_cast<const unsigned char*>(key));
        if (moved != 1) return {};
        auto value = str(api.value(reader)); api.element(reader); return value;
    };
    struct Capture { int depth{-1}; std::string kind, value; } capture;
    std::vector<std::string> stack;
    std::vector<bool> localized_stack;
    std::string component, modern, legacy;
    std::set<std::string> desktops;
    bool root = false, desktop = false, id_seen = false, modern_seen = false, legacy_seen = false, developer_seen = false;
    std::size_t nodes = 0;
    auto finish = [&]() -> bool {
        const auto value = trim(std::move(capture.value));
        if (capture.kind == "id") { if (id_seen || !valid_component_id(value)) return false; id_seen = true; component = value; }
        if (capture.kind == "modern") { if (modern_seen || !valid_label(value)) return false; modern_seen = true; modern = value; }
        if (capture.kind == "legacy") { if (legacy_seen || !valid_label(value)) return false; legacy_seen = true; legacy = value; }
        if (capture.kind == "launchable") {
            if (!valid_id(value) || !value.ends_with(".desktop") || !desktops.insert(value).second) return false;
        }
        capture = {}; return true;
    };
    int next{};
    while ((next = api.read(reader)) == 1) {
        if (++nodes > 100000) return absent("XML node limit exceeded");
        const auto type = api.type(reader), depth = api.depth(reader);
        if (depth < 0 || depth > 64) return absent("XML depth limit exceeded");
        if (type == 10 || type == 5 || type == 6) return absent("DTD/entity nodes forbidden");
        if (type == 1) {
            if (capture.depth >= 0) return absent("Nested markup in metadata identity/label");
            const auto name = str(api.name(reader));
            if (!str(api.ns(reader)).empty() || name.find(':') != name.npos) return absent("Namespaced metadata not supported");
            if (static_cast<std::size_t>(depth) != stack.size()) return absent("Inconsistent XML depth");
            if (!root) {
                if (depth != 0 || name != "component") return absent("Requires one upstream component, not a catalog");
                root = true; desktop = attribute("type") == "desktop-application";
            }
            stack.push_back(name);
            // Only the unlocalized default developer label is selected.
            const bool localized = (!localized_stack.empty() && localized_stack.back()) || attribute("xml:lang").has_value();
            localized_stack.push_back(localized);
            std::string kind;
            if (depth == 1 && name == "id") kind = "id";
            if (depth == 1 && name == "developer") {
                if (developer_seen) return absent("Duplicate developer element");
                developer_seen = true;
            }
            if (!localized && depth == 1 && name == "developer_name") kind = "legacy";
            if (!localized && depth == 2 && stack[1] == "developer" && name == "name") kind = "modern";
            if (depth == 1 && name == "launchable" && attribute("type") == "desktop-id") kind = "launchable";
            if (!kind.empty()) capture = {depth, kind, {}};
            if (api.empty(reader) == 1) {
                if (capture.depth == depth && !finish()) return absent("Empty/duplicate metadata identity or label");
                stack.pop_back();
                localized_stack.pop_back();
            }
        } else if (type == 15) {
            if (stack.empty() || static_cast<std::size_t>(depth + 1) != stack.size()) return absent("Invalid XML end element");
            if (capture.depth == depth && !finish()) return absent("Invalid/duplicate metadata identity or label");
            stack.pop_back();
            localized_stack.pop_back();
        } else if (capture.depth >= 0 && (type == 3 || type == 4 || type == 13 || type == 14)) {
            capture.value += str(api.value(reader));
            if (capture.value.size() > 4096) return absent("Metadata label/identity too long");
        } else if (type == 7) return absent("XML processing instructions not supported");
    }
    if (next != 0 || !root || !stack.empty() || capture.depth >= 0) return absent("Malformed/incomplete XML metadata");
    Rows rows;
    if (desktop) {
        if (!id_seen || (modern_seen && legacy_seen && modern != legacy)) return absent("Missing component ID or conflicting developer labels");
        const auto label = modern_seen ? modern : legacy;
        if (!label.empty()) for (const auto& id : desktops) rows.push_back({id, component, label, {}});
    }
    return {std::move(rows), Support::estimated, "AppStream declared developer; not a verified publisher or executable owner"};
#else
    return absent("AppStream reader requires Linux and optional system libxml2");
#endif
}
std::vector<std::filesystem::path> resolve_linux_metainfo_directories(
    std::string_view home, std::string_view data_home, std::string_view data_dirs) {
    if (home.size() > 32768 || data_home.size() > 32768 || data_dirs.size() > 1048576 ||
        home.find('\0') != home.npos || data_home.find('\0') != data_home.npos || data_dirs.find('\0') != data_dirs.npos)
        throw std::invalid_argument("Invalid/excessive XDG data environment");
    std::vector<std::filesystem::path> out; std::set<std::filesystem::path> seen;
    auto add = [&](std::string_view base) {
        if (base.empty() || base.front() != '/') return false;
        if (base.size() > 32768) throw std::invalid_argument("Excessive XDG data path");
        for (const auto* suffix : {"metainfo", "appdata"}) {
            auto path = (std::filesystem::path(base) / suffix).lexically_normal();
            if (seen.insert(path).second) {
                if (out.size() >= 128) throw std::invalid_argument("Too many XDG metadata directories");
                out.push_back(std::move(path));
            }
        }
        return true;
    };
    if (!add(data_home) && !home.empty() && home.front() == '/') add(std::string(home) + "/.local/share");
    bool system = false;
    while (!data_dirs.empty()) {
        const auto end = data_dirs.find(':'); system = add(data_dirs.substr(0, end)) || system;
        if (end == data_dirs.npos) break;
        data_dirs.remove_prefix(end + 1);
    }
    if (!system) { add("/usr/local/share"); add("/usr/share"); }
    return out;
}
Result read_linux_startup_publishers_from(const std::vector<std::filesystem::path>& directories) {
#ifdef __linux__
    if (!xml_api().ready) return absent("Optional system libxml2 reader unavailable");
    if (directories.size() > 128) return absent("Metadata directory limit exceeded");
    Rows rows; std::size_t visited = 0, payload = 0, output = 0;
    for (const auto& path : directories) {
        if (!path.is_absolute() || path.string().size() > 32768 || path.string().find('\0') != std::string::npos)
            return absent("Invalid metadata directory path");
        const int fd = ::open(path.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
        if (fd < 0) {
            if (errno == ENOENT) continue;
            return absent("Metadata directory inaccessible/unsupported", errno == EACCES || errno == EPERM ? Support::permission_required : Support::unsupported);
        }
        DIR* directory = ::fdopendir(fd);
        if (!directory) { ::close(fd); return absent("Metadata enumeration failed"); }
        struct CloseDir { DIR* value; ~CloseDir() { ::closedir(value); } } close_dir{directory};
        for (;;) {
            errno = 0; const auto* item = ::readdir(directory);
            if (!item) { if (errno) return absent("Metadata enumeration incomplete"); break; }
            const std::string name = item->d_name;
            if (name == "." || name == "..") continue;
            if (++visited > 8192) return absent("Metadata entry count exceeded");
            if (!name.ends_with(".metainfo.xml") && !name.ends_with(".appdata.xml")) continue;
            struct File { int fd; ~File() { if (fd >= 0) ::close(fd); } } file{
                ::openat(fd, name.c_str(), O_RDONLY | O_NONBLOCK | O_NOFOLLOW | O_CLOEXEC)};
            if (file.fd < 0) return absent("Metadata file inaccessible/unsupported", errno == EACCES || errno == EPERM ? Support::permission_required : Support::unsupported);
            struct stat before{}, after{}, named{};
            if (::fstat(file.fd, &before) != 0 || !S_ISREG(before.st_mode) || before.st_size < 0 || before.st_size > static_cast<off_t>(file_limit))
                return absent("Metadata requires regular file of at most 1MiB");
            std::string text; char buffer[4096];
            for (;;) {
                const auto n = ::read(file.fd, buffer, sizeof(buffer));
                if (n < 0) { if (errno == EINTR) continue; return absent("Metadata read failed"); }
                if (!n) break;
                if (static_cast<std::size_t>(n) > file_limit - text.size() || static_cast<std::size_t>(n) > total_limit - payload)
                    return absent("Metadata input byte limit exceeded");
                text.append(buffer, n); payload += n;
            }
            auto parsed = parse_linux_startup_metainfo(text);
            if (!parsed.value) return parsed; // incomplete scan must not publish a unique match
            if (::fstat(file.fd, &after) != 0 || ::fstatat(fd, name.c_str(), &named, AT_SYMLINK_NOFOLLOW) != 0 ||
                !same(before, after) || !same(before, named)) return absent("Metadata file changed during scan");
            for (auto& row : *parsed.value) {
                row.metadata_file = (path / name).string();
                output += row.desktop_file_id.size() + row.component_id.size() + row.developer_label.size() + row.metadata_file.size();
                if (rows.size() >= 8192 || output > total_limit) return absent("Metadata output limit exceeded");
                rows.push_back(std::move(row));
            }
        }
    }
    return {std::move(rows), Support::estimated, "Bounded non-atomic XDG upstream AppStream scan; declared developer, not verified signer"};
#else
    (void)directories; return absent("AppStream native filesystem scan requires Linux");
#endif
}
}
Metric<std::vector<LinuxStartupPublisher>> read_linux_startup_publishers() {
#ifdef __linux__
    try {
        auto env = [](const char* key) { const auto* value = std::getenv(key); return value ? std::string(value) : std::string{}; };
        auto home = env("HOME");
        if (home.empty() || home.front() != '/') if (const auto* account = ::getpwuid(::geteuid()); account && account->pw_dir) home = account->pw_dir;
        return detail::read_linux_startup_publishers_from(detail::resolve_linux_metainfo_directories(home, env("XDG_DATA_HOME"), env("XDG_DATA_DIRS")));
    } catch (const std::exception&) { return absent("Invalid/excessive XDG metadata context or scan failure"); }
#else
    return absent("Linux AppStream publisher provider unavailable on this platform");
#endif
}
void apply_linux_startup_publishers(std::vector<StartupEntry>& entries, const Result& report) {
    std::map<std::string, std::vector<const LinuxStartupPublisher*>> index;
    const bool usable = report.value && report.value->size() <= 8192 &&
        (report.support == Support::available || report.support == Support::estimated) &&
        std::all_of(report.value->begin(), report.value->end(), [](const auto& row) {
            return valid_id(row.desktop_file_id) && row.desktop_file_id.ends_with(".desktop") &&
                valid_component_id(row.component_id) && valid_label(row.developer_label) &&
                row.metadata_file.size() <= 32768 && row.metadata_file.find('\0') == std::string::npos;
        });
    if (usable) for (const auto& row : *report.value) index[row.desktop_file_id].push_back(&row);
    for (auto& entry : entries) {
        entry.publisher.clear(); entry.publisher_label = {std::nullopt, report.support, report.note};
        entry.publisher_source_executable = {std::nullopt, Support::unsupported, "AppStream metadata does not identify a verified executable publisher"};
        const auto id = std::filesystem::path(entry.id).filename().string();
        const auto found = index.find(id);
        if (entry.id.empty() || !valid_id(id) || !id.ends_with(".desktop") || entry.status == StartupStatus::unknown ||
            found == index.end() || found->second.size() != 1) {
            if (report.value) entry.publisher_label = {std::nullopt, Support::unsupported, "No unambiguous AppStream desktop-id/developer match; not inferred from name, Exec or username"};
            continue;
        }
        const auto& matched = *found->second.front();
        entry.publisher = matched.developer_label;
        entry.publisher_label = {matched.developer_label, Support::estimated,
            "AppStream self-reported developer; component=" + matched.component_id + "; metadata=" + matched.metadata_file + "; NOT a verified signer/executable association"};
    }
}
}
