#include "linux_efficiency.hpp"
#include "linux_process_stat.hpp"
#include <algorithm>
#include <cerrno>
#include <charconv>
#include <limits>
#include <set>
#ifdef __linux__
#include <dirent.h>
#include <fcntl.h>
#include <sys/vfs.h>
#include <unistd.h>
#endif

namespace pcinsight::detail {
namespace {
constexpr std::size_t max_threads = 4096;
constexpr int idle = 3 << 13;
bool valid_id(std::uint32_t id) { return id > 1 && id <= static_cast<std::uint32_t>(std::numeric_limits<int>::max()); }
bool valid_io(int v) {
    if (v < 0 || v > 0xffff) return false;
    const auto cls = v >> 13, data = v & 8191;
    return (cls == 0 && !data) || (cls >= 1 && cls <= 3 && data <= 7);
}
ActionResult error(int e, const char* stage) {
    auto kind = ActionError::system_error;
    if (e == EPERM || e == EACCES) kind = ActionError::permission_denied;
    else if (e == ESRCH || e == ENOENT) kind = ActionError::not_found;
    else if (e == ENOSYS || e == EOPNOTSUPP) kind = ActionError::not_supported;
    else if (e == EINVAL) kind = ActionError::invalid_argument;
    return {kind, std::string(stage) + "; errno " + std::to_string(e) + "; possible partial effects; no rollback"};
}
ActionResult conflict(const char* note) { return {ActionError::stale_target, note}; }
ActionResult identity(const LinuxThreadEfficiencyApi& api, std::uint32_t pid,
                      std::uint32_t tid, std::uint64_t ticks) {
    const auto got = api.identity(pid, tid);
    if (got.error) return error(got.error, "identity read failed");
    if (got.pid != tid || got.start_ticks != ticks) return conflict("observed identity differs; no further writes");
    return {};
}
ActionResult ready(std::uint32_t pid, std::uint64_t ticks, bool ack, const LinuxThreadEfficiencyApi& api) {
    if (!ack) return {ActionError::invalid_argument, "numeric PID/TID reuse risk must be explicitly acknowledged; no changes"};
    if (!valid_id(pid) || pid == api.priorities.self_pid || !ticks)
        return {ActionError::invalid_argument, "reserved/self PID or zero identity; no changes"};
    const auto& p = api.priorities;
    if (!api.check_context || !api.identity || !api.list || !p.read_nice || !p.read_io || !p.set_nice || !p.set_io)
        return {ActionError::not_supported, "incomplete thread priority API; no changes"};
    if (const auto e = api.check_context()) return error(e, "proc context rejected; no changes");
    return identity(api, pid, pid, ticks);
}
LinuxThreadList list(const LinuxThreadEfficiencyApi& api, std::uint32_t pid) {
    auto rows = api.list(pid);
    if (rows.error) return rows;
    if (rows.tids.empty() || rows.tids.size() > max_threads) return {{}, EOVERFLOW};
    std::sort(rows.tids.begin(), rows.tids.end());
    if (!std::binary_search(rows.tids.begin(), rows.tids.end(), pid) ||
        std::adjacent_find(rows.tids.begin(), rows.tids.end()) != rows.tids.end() ||
        !std::all_of(rows.tids.begin(), rows.tids.end(), valid_id)) return {{}, EINVAL};
    return rows;
}
// A field is changed independently: restoring I/O can succeed while CPU restore
// is denied. Pending bits survive failures, including failed read-back.
ActionResult field(const LinuxThreadEfficiencyApi& api, LinuxEfficiencyReceipt& receipt,
                   LinuxThreadPriorityChange& row, bool cpu, bool restore) {
    bool& pending = cpu ? row.nice_pending : row.io_pending;
    if (restore && !pending) return {};
    auto guard = identity(api, receipt.process_id, receipt.process_id, receipt.process_start_ticks);
    if (!guard) return guard;
    guard = identity(api, receipt.process_id, row.tid, row.start_ticks);
    if (!guard) return guard;
    const auto& read = cpu ? api.priorities.read_nice : api.priorities.read_io;
    const auto& write = cpu ? api.priorities.set_nice : api.priorities.set_io;
    const int before = cpu ? row.original_nice : row.original_io;
    const int requested = cpu ? row.requested_nice : row.requested_io;
    const int from = restore ? requested : before, to = restore ? before : requested;
    const auto current = read(row.tid);
    if (current.error) return error(current.error, "priority read failed");
    if (current.value == to) { if (restore) pending = false; return {}; }
    if (current.value != from) return conflict("observed priority conflict; no further writes (not an atomic comparison)");
    if (const auto e = write(row.tid, to)) return error(e, cpu ? "CPU write failed" : "I/O write failed");
    pending = true;
    const auto final = read(row.tid);
    if (final.error) return error(final.error, "write accepted; read-back failed");
    if (final.value != to) return {ActionError::system_error, "write accepted; read-back differs; pending receipt retained"};
    guard = identity(api, receipt.process_id, row.tid, row.start_ticks);
    if (!guard) return guard;
    if (restore) pending = false;
    return {};
}
}

LinuxEfficiencyUpdate apply_linux_efficiency_unchecked(std::uint32_t pid, std::uint64_t ticks,
        bool ack, const LinuxThreadEfficiencyApi& api) {
    LinuxEfficiencyUpdate out;
    out.result = ready(pid, ticks, ack, api);
    if (!out.result) return out;
    const auto first = list(api, pid);
    if (first.error) { out.result = error(first.error, "thread enumeration failed; no changes"); return out; }
    out.receipt.process_id = pid; out.receipt.process_start_ticks = ticks;
    for (auto tid : first.tids) {
        const auto id = api.identity(pid, tid);
        if (id.error) { out.result = error(id.error, "thread identity capture failed; no changes"); return out; }
        if (id.pid != tid || !id.start_ticks) { out.result = conflict("invalid thread identity; no changes"); return out; }
        const auto n = api.priorities.read_nice(tid), io = api.priorities.read_io(tid);
        if (n.error || io.error) { out.result = error(n.error ? n.error : io.error, "priority capture failed; no changes"); return out; }
        if (n.value < -20 || n.value > 19 || !valid_io(io.value)) {
            out.result = {ActionError::system_error, "invalid native priorities; no changes"}; return out;
        }
        out.result = identity(api, pid, tid, id.start_ticks);
        if (!out.result) return out;
        out.receipt.threads.push_back({tid, id.start_ticks, n.value, io.value, std::max(n.value, 10), idle});
    }
    const auto second = list(api, pid);
    if (second.error || first.tids != second.tids) {
        out.result = second.error ? error(second.error, "thread recapture failed; no changes") : conflict("thread set changed during capture; no changes");
        return out;
    }
    for (auto& row : out.receipt.threads) {
        out.result = field(api, out.receipt, row, false, false);
        if (out.result) out.result = field(api, out.receipt, row, true, false);
        if (!out.result) return out;
    }
    // Verify every captured row again: shared I/O contexts and external writers
    // can invalidate an earlier observation. Still no atomic snapshot guarantee.
    for (const auto& row : out.receipt.threads) {
        out.result = identity(api, pid, row.tid, row.start_ticks);
        if (!out.result) return out;
        const auto n = api.priorities.read_nice(row.tid), io = api.priorities.read_io(row.tid);
        if (n.error || io.error) { out.result = error(n.error ? n.error : io.error, "final priority read failed"); return out; }
        if (n.value != row.requested_nice || io.value != row.requested_io) {
            out.result = {ActionError::system_error, "final captured thread priorities differ; partial effects retained"}; return out;
        }
    }
    const auto last = list(api, pid);
    out.result = identity(api, pid, pid, ticks);
    if (!out.result) return out;
    if (last.error || first.tids != last.tids) {
        out.result = last.error ? error(last.error, "final thread enumeration failed") : conflict("thread set changed after writes; partial coverage; receipt retained"); return out;
    }
    out.result = {ActionError::none, "captured thread preset observed; unchecked numeric TIDs; no future-thread/atomic/energy guarantee"};
    return out;
}

LinuxEfficiencyUpdate restore_linux_efficiency_unchecked(const LinuxEfficiencyReceipt& receipt,
        bool ack, const LinuxThreadEfficiencyApi& api) {
    LinuxEfficiencyUpdate out;
    // Validate the ENTIRE receipt before any native read or write.
    if (receipt.threads.empty() || receipt.threads.size() > max_threads) {
        out.result = {ActionError::invalid_argument, "empty/oversized receipt; no changes"}; return out;
    }
    std::set<std::uint32_t> seen;
    bool valid = true;
    for (const auto& row : receipt.threads) {
        valid = valid && valid_id(row.tid) && row.tid != api.priorities.self_pid && row.start_ticks &&
            seen.insert(row.tid).second && row.original_nice >= -20 && row.original_nice <= 19 &&
            row.requested_nice == std::max(row.original_nice, 10) && valid_io(row.original_io) && row.requested_io == idle &&
            (!row.nice_pending || row.original_nice != row.requested_nice) &&
            (!row.io_pending || row.original_io != row.requested_io);
        if (row.tid == receipt.process_id && row.start_ticks != receipt.process_start_ticks) valid = false;
    }
    if (!valid || !seen.contains(receipt.process_id)) {
        out.result = {ActionError::invalid_argument, "invalid receipt; no changes"}; return out;
    }
    out.receipt = receipt;
    out.result = ready(receipt.process_id, receipt.process_start_ticks, ack, api);
    if (!out.result) return out;
    for (auto& row : out.receipt.threads) {
        out.result = field(api, out.receipt, row, false, true);
        if (out.result) out.result = field(api, out.receipt, row, true, true);
        if (!out.result) return out;
    }
    out.result = identity(api, receipt.process_id, receipt.process_id, receipt.process_start_ticks);
    if (!out.result) return out;
    out.result = {ActionError::none, "recorded writes restored/observed independently; unchecked TIDs; no atomic or future-thread guarantee"};
    return out;
}

#ifdef __linux__
namespace {
LinuxIdentityResult native_identity(std::uint32_t pid, std::uint32_t tid) {
    const auto path = "/proc/" + std::to_string(pid) + "/task/" + std::to_string(tid) + "/stat";
    const int fd = ::open(path.c_str(), O_RDONLY | O_NOFOLLOW | O_CLOEXEC | O_NONBLOCK);
    if (fd < 0) return {0, 0, errno};
    struct Close { int fd; ~Close() { ::close(fd); } } guard{fd};
    std::string text; char buffer[4096]; unsigned interrupts = 0;
    for (;;) {
        const auto n = ::read(fd, buffer, sizeof buffer);
        if (n < 0) { const int e = errno; if (e == EINTR && ++interrupts <= 3) continue; return {0, 0, e}; }
        if (!n) break;
        if (static_cast<std::size_t>(n) > 65536 - text.size()) return {0, 0, EOVERFLOW};
        text.append(buffer, static_cast<std::size_t>(n));
    }
    const auto stat = parse_linux_process_stat(text);
    return stat ? LinuxIdentityResult{stat->pid, stat->start_ticks, 0} : LinuxIdentityResult{0, 0, EIO};
}
LinuxThreadEfficiencyApi native_api() {
    LinuxThreadEfficiencyApi api;
    api.priorities = native_linux_efficiency_api();
    api.identity = native_identity;
    api.check_context = [] {
        struct statfs info{};
        if (::statfs("/proc", &info)) return errno;
        if (info.f_type != 0x9fa0) return EOPNOTSUPP;
        // /proc/self must refer to the same PID namespace as numeric syscalls.
        const auto self = ::getpid(); char link[32];
        const auto n = ::readlink("/proc/self", link, sizeof link);
        if (n < 0) return errno;
        return std::string_view(link, static_cast<std::size_t>(n)) == std::to_string(self) ? 0 : EOPNOTSUPP;
    };
    api.list = [](std::uint32_t pid) -> LinuxThreadList {
        const auto path = "/proc/" + std::to_string(pid) + "/task";
        auto* dir = ::opendir(path.c_str());
        if (!dir) return {{}, errno};
        struct Close { DIR* d; ~Close() { ::closedir(d); } } guard{dir};
        LinuxThreadList out; unsigned entries = 0;
        for (;;) {
            errno = 0; const auto* row = ::readdir(dir);
            if (!row) { out.error = errno; return out; }
            if (++entries > max_threads + 2) return {{}, EOVERFLOW};
            const std::string_view name(row->d_name);
            if (name == "." || name == "..") continue;
            std::uint32_t tid{};
            const auto [end, ec] = std::from_chars(name.data(), name.data() + name.size(), tid);
            if (ec != std::errc{} || end != name.data() + name.size() || !valid_id(tid)) return {{}, EIO};
            out.tids.push_back(tid);
        }
    };
    return api;
}
}
#endif
} // namespace pcinsight::detail

namespace pcinsight {
LinuxEfficiencyUpdate apply_linux_efficiency_unchecked(std::uint32_t pid, std::uint64_t ticks, bool ack) {
#ifdef __linux__
    return detail::apply_linux_efficiency_unchecked(pid, ticks, ack, detail::native_api());
#else
    (void)pid; (void)ticks; (void)ack;
    return {{ActionError::not_supported, "Linux required"}, {}};
#endif
}
LinuxEfficiencyUpdate restore_linux_efficiency_unchecked(const LinuxEfficiencyReceipt& receipt, bool ack) {
#ifdef __linux__
    return detail::restore_linux_efficiency_unchecked(receipt, ack, detail::native_api());
#else
    (void)receipt; (void)ack;
    return {{ActionError::not_supported, "Linux required"}, {}};
#endif
}
}
