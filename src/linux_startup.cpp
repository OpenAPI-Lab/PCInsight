#include "linux_startup.hpp"
#include <algorithm>
#include <atomic>
#include <filesystem>
#include <vector>
#ifdef __linux__
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <unistd.h>
#endif
namespace pcinsight::detail {
namespace {
constexpr std::size_t limit = 1024 * 1024;
std::string_view trim(std::string_view value) {
    while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) value.remove_prefix(1);
    while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) value.remove_suffix(1);
    return value;
}
}
std::optional<std::string> rewrite_xdg_startup(std::string_view text, bool enabled) {
    if (text.empty() || text.size() > limit || text.find('\0') != text.npos) return {};
    const auto first_newline = text.find('\n');
    const std::string newline = first_newline != text.npos && first_newline && text[first_newline - 1] == '\r' ? "\r\n" : "\n";
    std::string out;
    bool main = false, found = false, hidden = false, gnome = false;
    auto insert = [&] {
        if (!hidden || !gnome) {
            if (!out.empty() && out.back() != '\n') out += newline;
            if (!hidden) out += std::string("Hidden=") + (enabled ? "false" : "true") + newline;
            if (!gnome) out += std::string("X-GNOME-Autostart-enabled=") + (enabled ? "true" : "false") + newline;
        }
    };
    while (!text.empty()) {
        auto end = text.find('\n');
        auto raw = text.substr(0, end == text.npos ? text.size() : end + 1);
        auto content = raw;
        if (!content.empty() && content.back() == '\n') content.remove_suffix(1);
        if (!content.empty() && content.back() == '\r') content.remove_suffix(1);
        auto line = trim(content);
        if (!line.empty() && line.front() == '[') {
            if (line.size() < 3 || line.back() != ']' || (!found && line != "[Desktop Entry]")) return {};
            if (main) insert();
            main = line == "[Desktop Entry]";
            if (main) { if (found) return {}; found = true; }
        }
        if (!found && !line.empty() && line.front() != '#') return {};
        bool changed = false;
        if (main && !line.empty() && line.front() != '#' && line.front() != '[') {
            const auto equals = content.find('=');
            if (equals != content.npos) {
                const auto key = trim(content.substr(0, equals));
                if (key == "Hidden" || key == "X-GNOME-Autostart-enabled") {
                    auto& seen = key == "Hidden" ? hidden : gnome;
                    if (seen) return {}; // ambiguous duplicate keys, never silently normalize
                    seen = true;
                    out.append(content.substr(0, equals + 1));
                    out += key == "Hidden" ? (enabled ? "false" : "true") : (enabled ? "true" : "false");
                    out.append(raw.substr(content.size()));
                    changed = true;
                }
            }
        }
        if (!changed) out.append(raw);
        if (end == text.npos) break;
        text.remove_prefix(end + 1);
    }
    if (!found) return {};
    if (main) insert();
    if (out.size() > limit) return {};
    return out;
}
std::optional<StartupEntry> read_xdg_startup(std::string_view text) {
    if (!rewrite_xdg_startup(text, true)) return {}; // same bounded structural rules
    StartupEntry entry; entry.status = StartupStatus::enabled;
    bool main = false, name_seen = false, exec_seen = false;
    while (!text.empty()) {
        const auto end = text.find('\n');
        auto line = text.substr(0, end);
        if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
        line = trim(line);
        if (!line.empty() && line.front() == '[') main = line == "[Desktop Entry]";
        if (main && !line.empty() && line.front() != '#' && line.front() != '[') {
            const auto equals = line.find('=');
            if (equals != line.npos) {
                const auto key = trim(line.substr(0, equals)), value = trim(line.substr(equals + 1));
                if (key == "Name") { if (name_seen) return {}; name_seen = true; entry.name = value; }
                else if (key == "Exec") { if (exec_seen) return {}; exec_seen = true; entry.command = value; }
                else if (key == "Hidden" || key == "X-GNOME-Autostart-enabled") {
                    if (value != "true" && value != "false") return {};
                    if ((key == "Hidden" && value == "true") || (key == "X-GNOME-Autostart-enabled" && value == "false"))
                        entry.status = StartupStatus::disabled;
                }
            }
        }
        if (end == text.npos) break;
        text.remove_prefix(end + 1);
    }
    return entry;
}
#ifdef __linux__
namespace {
struct Fd { int value{-1}; ~Fd() { if (value >= 0) close(value); } };
ActionResult fail(const char* action, int code = errno) {
    auto error = code == EACCES || code == EPERM ? ActionError::permission_denied : ActionError::system_error;
    return {error, std::string(action) + ": " + std::strerror(code)};
}
bool same(const struct stat& a, const struct stat& b) {
    return a.st_dev == b.st_dev && a.st_ino == b.st_ino && a.st_size == b.st_size &&
        a.st_mtim.tv_sec == b.st_mtim.tv_sec && a.st_mtim.tv_nsec == b.st_mtim.tv_nsec &&
        a.st_ctim.tv_sec == b.st_ctim.tv_sec && a.st_ctim.tv_nsec == b.st_ctim.tv_nsec;
}
std::atomic<unsigned long long> temporary_id{};
}
#endif
ActionResult set_linux_startup_enabled(const std::string& entry, bool enabled) {
    if (entry.find('\0') != entry.npos || std::filesystem::path(entry).extension() != ".desktop")
        return {ActionError::invalid_argument, "expected .desktop path without NUL"};
#ifdef __linux__
    const std::filesystem::path path(entry);
    const auto name = path.filename().string();
    auto parent = path.parent_path(); if (parent.empty()) parent = ".";
    Fd dir{open(parent.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC)};
    if (dir.value < 0) return fail("open startup directory");
    if (flock(dir.value, LOCK_EX | LOCK_NB) != 0) {
        if (errno == EWOULDBLOCK) return {ActionError::stale_target, "another startup edit holds this directory"};
        return fail("lock startup directory");
    }
    Fd original{openat(dir.value, name.c_str(), O_RDONLY | O_NOFOLLOW | O_CLOEXEC | O_NONBLOCK)};
    if (original.value < 0) {
        if (errno == ELOOP) return {ActionError::invalid_argument, "startup symlinks are not editable"};
        if (errno == ENOENT) return {ActionError::not_found, "startup entry not found"};
        return fail("open startup entry");
    }
    struct stat before{};
    if (fstat(original.value, &before) != 0) return fail("stat startup entry");
    if (!S_ISREG(before.st_mode) || before.st_nlink != 1 || before.st_size < 0 || before.st_size > static_cast<off_t>(limit))
        return {ActionError::invalid_argument, "startup must be a single-link regular file of at most 1 MiB"};
    std::string bytes; char chunk[4096];
    for (;;) {
        const auto n = read(original.value, chunk, sizeof(chunk));
        if (n < 0) { if (errno == EINTR) continue; return fail("read startup entry"); }
        if (!n) break;
        bytes.append(chunk, static_cast<std::size_t>(n));
        if (bytes.size() > limit) return {ActionError::invalid_argument, "startup file grew beyond limit"};
    }
    auto rewritten = rewrite_xdg_startup(bytes, enabled);
    if (!rewritten) return {ActionError::invalid_argument, "missing/duplicate Desktop Entry or flags, malformed/oversize file"};
    struct stat current{};
    if (fstat(original.value, &current) != 0) return fail("restat startup entry");
    if (!same(before, current)) return {ActionError::stale_target, "startup changed while reading"};
    struct Attribute { std::string name; std::vector<char> value; };
    std::vector<Attribute> attributes;
    const auto attribute_size = flistxattr(original.value, nullptr, 0);
    if (attribute_size < 0 && errno != ENOTSUP) return fail("list startup metadata");
    if (attribute_size > 65536) return {ActionError::not_supported, "startup metadata exceeds limit"};
    if (attribute_size > 0) {
        std::vector<char> names(static_cast<std::size_t>(attribute_size));
        const auto count = flistxattr(original.value, names.data(), names.size());
        if (count != attribute_size) return {ActionError::stale_target, "startup metadata changed"};
        std::size_t offset = 0, total = 0;
        while (offset < names.size()) {
            const auto length = strnlen(names.data() + offset, names.size() - offset);
            if (!length || length == names.size() - offset) return {ActionError::system_error, "invalid attribute list"};
            Attribute attribute{{names.data() + offset, length}, {}};
            const auto size = fgetxattr(original.value, attribute.name.c_str(), nullptr, 0);
            if (size < 0) return fail("read startup metadata");
            if (size > 65536 || total + static_cast<std::size_t>(size) > limit)
                return {ActionError::not_supported, "startup attributes exceed limit"};
            attribute.value.resize(static_cast<std::size_t>(size)); total += attribute.value.size();
            if (fgetxattr(original.value, attribute.name.c_str(), attribute.value.data(), attribute.value.size()) != size)
                return {ActionError::stale_target, "startup attribute changed"};
            attributes.push_back(std::move(attribute)); offset += length + 1;
        }
    }
    Fd temporary;
    std::string temp_name;
    for (unsigned attempt = 0; attempt < 128; ++attempt) {
        temp_name = ".pcinsight-startup-" + std::to_string(getpid()) + "-" + std::to_string(temporary_id.fetch_add(1));
        temporary.value = openat(dir.value, temp_name.c_str(), O_CREAT | O_EXCL | O_NOFOLLOW | O_WRONLY | O_CLOEXEC, 0600);
        if (temporary.value >= 0) break;
        if (errno != EEXIST) return fail("create startup temporary");
    }
    if (temporary.value < 0) return {ActionError::system_error, "startup temporary collisions"};
    struct Cleanup { int directory; std::string name; bool active{true}; ~Cleanup() { if (active) unlinkat(directory, name.c_str(), 0); } } cleanup{dir.value, temp_name};
    std::size_t offset = 0;
    while (offset < rewritten->size()) {
        const auto n = write(temporary.value, rewritten->data() + offset, rewritten->size() - offset);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return fail("write startup temporary", n < 0 ? errno : EIO);
        offset += static_cast<std::size_t>(n);
    }
    if (fchown(temporary.value, before.st_uid, before.st_gid) != 0) return fail("preserve startup ownership");
    // Default ACLs/labels may be inherited by the temporary; don't accidentally
    // retain extra attributes that the selected original did not have.
    const auto inherited_size = flistxattr(temporary.value, nullptr, 0);
    if (inherited_size < 0 && errno != ENOTSUP) return fail("list inherited startup metadata");
    if (inherited_size > 65536) return {ActionError::not_supported, "inherited metadata exceeds limit"};
    if (inherited_size > 0) {
        std::vector<char> inherited(static_cast<std::size_t>(inherited_size));
        if (flistxattr(temporary.value, inherited.data(), inherited.size()) != inherited_size)
            return fail("read inherited startup metadata");
        for (std::size_t index = 0; index < inherited.size();) {
            const auto length = strnlen(inherited.data() + index, inherited.size() - index);
            if (!length || length == inherited.size() - index) return {ActionError::system_error, "invalid inherited metadata list"};
            std::string inherited_name(inherited.data() + index, length);
            if (std::none_of(attributes.begin(), attributes.end(), [&](const auto& attr) { return attr.name == inherited_name; }))
                if (fremovexattr(temporary.value, inherited_name.c_str()) != 0) return fail("remove extra inherited attribute");
            index += length + 1;
        }
    }
    for (const auto& attr : attributes)
        if (fsetxattr(temporary.value, attr.name.c_str(), attr.value.data(), attr.value.size(), 0) != 0) return fail("preserve startup attribute");
    if (fchmod(temporary.value, before.st_mode & 07777) != 0) return fail("preserve startup permissions");
    if (fsync(temporary.value) != 0) return fail("sync startup temporary");
    const int completed_fd = temporary.value; temporary.value = -1;
    if (close(completed_fd) != 0) return fail("close startup temporary");
    if (fstatat(dir.value, name.c_str(), &current, AT_SYMLINK_NOFOLLOW) != 0) return fail("recheck startup target");
    if (!same(before, current)) return {ActionError::stale_target, "startup target changed before commit"};
    if (renameat(dir.value, temp_name.c_str(), dir.value, name.c_str()) != 0) return fail("replace startup entry");
    cleanup.active = false;
    if (fsync(dir.value) != 0) return fail("startup flags applied, but directory sync failed");
    return {};
#else
    (void)enabled;
    return {ActionError::not_supported, "XDG startup editing requires Linux"};
#endif
}
}
