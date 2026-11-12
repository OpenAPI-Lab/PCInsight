#include "linux_filesystem_capacity.hpp"
#include <algorithm>
#include <charconv>
#include <set>
#include <tuple>
#ifdef __linux__
#include <cerrno>
#include <dirent.h>
#include <fcntl.h>
#include <linux/magic.h>
#include <sys/vfs.h>
#include <unistd.h>
#endif
namespace pcinsight {
namespace {
bool name_valid(const std::string& name) {
    return !name.empty() && name.size() <= 255 && name != "." && name != ".." &&
        name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._-") == name.npos;
}
std::optional<std::uint64_t> number(std::string_view text) {
    if (text.empty() || text.size() > 20) return {};
    std::uint64_t value{}; const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size() || std::to_string(value) != text) return {};
    return value;
}
bool device_valid(const std::string& text) {
    const auto colon = text.find(':'); if (colon == text.npos) return false;
    const auto major = number(std::string_view(text).substr(0, colon)), minor = number(std::string_view(text).substr(colon + 1));
    return major && minor && *major <= UINT32_MAX && *minor <= UINT32_MAX;
}
template<class T> Metric<T> unknown(const char* note, Support support = Support::unsupported) { return {std::nullopt, support, note}; }
bool validate(detail::LinuxFilesystemLayout& layout, const std::string& id) {
    if (layout.disk != id || !device_valid(layout.device) || !layout.size || layout.regions.empty() || layout.regions.size() > 128 ||
        (!layout.sequence.empty() && !number(layout.sequence))) return false;
    std::set<std::string> devices, names;
    for (const auto& region : layout.regions) {
        if (!name_valid(region.name) || !device_valid(region.device) || !region.size || region.offset > layout.size ||
            region.size > layout.size - region.offset || !devices.insert(region.device).second || !names.insert(region.name).second) return false;
        if ((region.device == layout.device || region.name == id) &&
            (layout.regions.size() != 1 || region.name != id || region.device != layout.device || region.offset || region.size != layout.size)) return false;
    }
    std::sort(layout.regions.begin(), layout.regions.end(), [](const auto& a, const auto& b) {
        return std::tie(a.offset, a.size, a.name, a.device) < std::tie(b.offset, b.size, b.name, b.device);
    });
    return true;
}
#ifdef __linux__
struct Fd { int fd{-1}; ~Fd() { if (fd >= 0) ::close(fd); } };
std::optional<std::string> attribute(int directory, const char* name) {
    Fd file{::openat(directory, name, O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK)};
    if (file.fd < 0) return {};
    char bytes[256]; const auto n = ::read(file.fd, bytes, sizeof(bytes));
    if (n <= 0 || n >= static_cast<ssize_t>(sizeof(bytes))) { errno = EIO; return {}; }
    std::string out(bytes, n); if (!out.empty() && out.back() == '\n') out.pop_back();
    return out;
}
Metric<detail::LinuxFilesystemLayout> layout_native(const std::string& id) {
    using Layout = detail::LinuxFilesystemLayout;
    Fd directory{::open(("/sys/block/" + id).c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC)};
    auto failure = [] { return unknown<Layout>("Selected sysfs disk/partition inventory unavailable or inconsistent",
        errno == EACCES || errno == EPERM ? Support::permission_required : Support::unsupported); };
    if (directory.fd < 0) return failure();
    struct statfs type{};
    if (::fstatfs(directory.fd, &type) < 0 || type.f_type != SYSFS_MAGIC) return unknown<Layout>("Block inventory requires kernel sysfs");
    Layout out; out.disk = id;
    const auto dev = attribute(directory.fd, "dev"), sectors = attribute(directory.fd, "size");
    if (!dev || !sectors) return failure();
    const auto count = number(*sectors);
    if (!count || *count > UINT64_MAX / 512) return unknown<Layout>("Invalid sysfs disk sector count");
    out.device = *dev; out.size = *count * 512;
    const auto sequence = attribute(directory.fd, "diskseq");
    if (sequence) out.sequence = *sequence;
    else if (errno != ENOENT) return failure();
    const int copy = ::dup(directory.fd); if (copy < 0) return failure();
    DIR* list = ::fdopendir(copy); if (!list) { ::close(copy); return failure(); }
    struct Close { DIR* list; ~Close() { ::closedir(list); } } close{list};
    std::size_t visited = 0;
    for (;;) {
        errno = 0; const auto* item = ::readdir(list);
        if (!item) { if (errno) return failure(); break; }
        const std::string name = item->d_name; if (name == "." || name == "..") continue;
        if (++visited > 4096) return unknown<Layout>("Sysfs disk enumeration limit exceeded");
        Fd child{::openat(directory.fd, name.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC)};
        if (child.fd < 0) { if (errno == ENOTDIR || errno == ELOOP) continue; return failure(); }
        const auto partition = attribute(child.fd, "partition");
        if (!partition) { if (errno == ENOENT) continue; return failure(); }
        const auto partition_number = number(*partition);
        if (!partition_number || !*partition_number) return unknown<Layout>("Invalid sysfs partition index");
        const auto start = attribute(child.fd, "start"), length = attribute(child.fd, "size"), device = attribute(child.fd, "dev");
        if (!start || !length || !device) return failure();
        const auto first = number(*start), size = number(*length);
        if (!first || !size || *first > UINT64_MAX / 512 || *size > UINT64_MAX / 512)
            return unknown<Layout>("Invalid sysfs partition sector range");
        out.regions.push_back({name, *device, *first * 512, *size * 512});
        if (out.regions.size() > 128) return unknown<Layout>("Sysfs partition count exceeded");
    }
    if (out.regions.empty()) out.regions.push_back({id, out.device, 0, out.size});
    if (!validate(out, id)) return unknown<Layout>("Invalid sysfs disk/partition layout");
    return {std::move(out), Support::estimated, "Kernel logical disk partitions, or whole object when no partitions; no MBR/GPT/raw scans or physical backing reconstruction"};
}
#endif
}
namespace detail {
Metric<LinuxDiskFilesystemCapacity> read_linux_disk_filesystem_capacity_with_api(const std::string& id, const LinuxFilesystemDiskApi& api) {
    if (!name_valid(id) || !api.layout || !api.probe) return unknown<LinuxDiskFilesystemCapacity>("Invalid disk identifier or incomplete filesystem provider");
    auto before = api.layout(id);
    if (!metric_is_known(before)) return unknown<LinuxDiskFilesystemCapacity>("Filesystem disk layout unavailable", before.support);
    if (!validate(*before.value, id)) return unknown<LinuxDiskFilesystemCapacity>("Filesystem disk layout invalid");
    LinuxDiskFilesystemCapacity report; report.disk_inventory_id = id;
    bool complete = true, overlap = false; std::uint64_t end = 0, sum = 0;
    Support missing_support = Support::unsupported;
    for (const auto& region : before.value->regions) {
        if (region.offset < end) overlap = true;
        end = std::max(end, region.offset + region.size);
        LinuxFilesystemTarget target; target.path = "/dev/" + region.name; target.region_length_bytes = region.size;
        target.allow_block_device = true; target.expected_kernel_block_device = region.device;
        auto measured = api.probe(target);
        if (metric_is_known(measured) && (!measured.value->source_is_block_device || measured.value->source_path != target.path ||
            measured.value->kernel_block_device.value != region.device || measured.value->source_size_bytes != region.size ||
            measured.value->region_offset_bytes != 0 || measured.value->region_length_bytes != region.size ||
            measured.value->declared_capacity_bytes < 2048 || measured.value->declared_capacity_bytes > region.size ||
            (measured.value->disk_inventory_id.value && measured.value->disk_inventory_id.value != id)))
            measured = unknown<LinuxFilesystemCapacity>("Filesystem region source/size/association differs from selected layout");
        if (!metric_is_known(measured)) {
            complete = false;
            if (measured.support == Support::permission_required) missing_support = Support::permission_required;
        } else if (measured.value->declared_capacity_bytes > UINT64_MAX - sum) complete = false;
        else sum += measured.value->declared_capacity_bytes;
        report.regions.push_back({std::move(target), region.offset, std::move(measured)});
    }
    auto after = api.layout(id);
    if (!metric_is_known(after)) return unknown<LinuxDiskFilesystemCapacity>("Filesystem disk layout revalidation failed", after.support);
    if (!validate(*after.value, id) || *before.value != *after.value)
        return unknown<LinuxDiskFilesystemCapacity>("Filesystem disk/partitions/diskseq changed; retry without stale total");
    if (complete && !overlap) report.declared_total_bytes = {sum, Support::estimated,
        "Sum of supported declared ext filesystem extents in ALL selected non-overlapping kernel regions; includes metadata, not mounted statvfs or physical backing capacity"};
    else report.declared_total_bytes = {std::nullopt, missing_support,
        overlap ? "Overlapping kernel regions: no aggregate total inferred" : "At least one region unknown/inaccessible/inconsistent/overflowed: no partial sum or zero total"};
    return {std::move(report), Support::estimated, "Selected logical disk region report; per-region availability, NOT physical layer reconstruction; matching endpoints are not an atomic snapshot"};
}
}
Metric<LinuxDiskFilesystemCapacity> read_linux_disk_filesystem_capacity(const std::string& id, bool allow) {
    if (!name_valid(id)) return unknown<LinuxDiskFilesystemCapacity>("Invalid disk inventory ID");
    if (!allow) return unknown<LinuxDiskFilesystemCapacity>("Explicit block-device read opt-in required; no raw probing performed");
#ifdef __linux__
    return detail::read_linux_disk_filesystem_capacity_with_api(id, {layout_native, read_linux_filesystem_capacity});
#else
    return unknown<LinuxDiskFilesystemCapacity>("Native selected-disk filesystem query requires Linux");
#endif
}
}
