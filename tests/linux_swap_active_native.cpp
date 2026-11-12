// MANUAL ONLY. Requires separate human authorization; never registered in CTest.
#include "../src/linux_swap_disks.hpp"
#include "../src/linux_formatted_capacity.hpp"
#include "pcinsight/monitor.hpp"
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <fcntl.h>
#include <linux/magic.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/swap.h>
#include <sys/sysmacros.h>
#include <sys/vfs.h>
#include <unistd.h>
using namespace pcinsight;
using namespace pcinsight::detail;
namespace {
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
std::vector<LinuxSwapEntry> inventory() {
    std::ifstream input("/proc/swaps");
    std::string text((std::istreambuf_iterator<char>(input)), {});
    auto parsed = parse_linux_swaps(text);
    require(input.good() && parsed.value.has_value(), "Cannot verify /proc/swaps");
    return *parsed.value;
}
bool contains(const std::vector<LinuxSwapEntry>& rows, const std::string& path) {
    return std::any_of(rows.begin(), rows.end(), [&](const auto& e) { return e.path == path; });
}
void preserved(const std::vector<LinuxSwapEntry>& before, const std::vector<LinuxSwapEntry>& after) {
    for (const auto& e : before) require(std::any_of(after.begin(), after.end(), [&](const auto& a) {
        return a.path == e.path && a.file == e.file && a.size_kib == e.size_kib && a.priority == e.priority;
    }), "Existing swap configuration changed");
}
struct PrivateFile {
    std::string directory, path;
    int fd{-1};
    bool attempted{}, active{}, cleaned{};
    struct stat identity{};
    bool cleanup() noexcept {
        if (cleaned) return true;
        try {
            // If activation returned an error, verify that it did not commit.
            if (attempted && !active) active = contains(inventory(), path);
            if (active) {
                if (::swapoff(path.c_str()) != 0) {
                    std::cerr << "RETAINED: swapoff failed errno=" << errno << " path=" << path << '\n';
                    return false;
                }
                active = false;
            }
            if (attempted && contains(inventory(), path)) {
                std::cerr << "RETAINED: still listed: " << path << '\n'; return false;
            }
            if (fd >= 0) {
                struct stat named{};
                if (::lstat(path.c_str(), &named) != 0 || named.st_dev != identity.st_dev ||
                    named.st_ino != identity.st_ino || !S_ISREG(named.st_mode)) {
                    std::cerr << "RETAINED: identity changed: " << path << '\n'; return false;
                }
                if (::unlink(path.c_str()) != 0) return false;
                ::close(fd); fd = -1;
            }
            if (!directory.empty() && ::rmdir(directory.c_str()) != 0) return false;
            cleaned = true;
            return true;
        } catch (...) {
            std::cerr << "RETAINED: cleanup cannot verify inventory: " << path << '\n'; return false;
        }
    }
    ~PrivateFile() { if (!cleaned) cleanup(); if (fd >= 0) ::close(fd); }
};
}
int main(int argc, char** argv) {
    if (argc != 2 || std::string(argv[1]) != "--authorized-private-swap") {
        std::cerr << "Manual authorized test only; no files created\n"; return 77;
    }
    PrivateFile file;
    try {
        require(::geteuid() == 0, "Root required for authorized swap test");
        constexpr std::uint64_t bytes = 64 * 1024 * 1024;
        const auto page = ::sysconf(_SC_PAGESIZE);
        require(page >= 4096 && page <= 65536 && bytes % page == 0, "Unsupported page size");
        std::ifstream mem("/proc/meminfo");
        std::string line; std::uint64_t available = 0;
        while (std::getline(mem, line)) if (line.starts_with("MemAvailable:")) available = std::stoull(line.substr(13));
        require(available >= 512 * 1024, "Insufficient MemAvailable for bounded test");
        struct statvfs space{};
        require(::statvfs("/tmp", &space) == 0 && space.f_bavail > (bytes * 4) / space.f_frsize,
                "Insufficient filesystem space");
        const auto before = inventory();
        char pattern[] = "/tmp/pcinsight-active-swap-XXXXXX";
        const auto made = ::mkdtemp(pattern); require(made, "mkdtemp failed");
        file.directory = made; file.path = file.directory + "/private.swap";
        file.fd = ::open(file.path.c_str(), O_RDWR | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600);
        require(file.fd >= 0, "Private file create failed");
        require(::fstat(file.fd, &file.identity) == 0, "fstat failed");
        struct statfs fs{};
        require(::fstatfs(file.fd, &fs) == 0 && fs.f_type == EXT2_SUPER_MAGIC, "Requires ext2/3/4");
        auto disk = resolve_linux_block_device(std::to_string(major(file.identity.st_dev)) + ":" +
                                               std::to_string(minor(file.identity.st_dev)));
        require(disk.has_value(), "No direct block mapping");
        require(::posix_fallocate(file.fd, 0, bytes) == 0, "Allocation failed");
        std::vector<char> zeros(1024 * 1024);
        for (std::uint64_t offset = 0; offset < bytes; offset += zeros.size())
            require(::pwrite(file.fd, zeros.data(), zeros.size(), offset) == static_cast<ssize_t>(zeros.size()), "Write failed");
        // Linux v6.6 include/linux/swap.h: native-endian version/last_page,
        // zero badpages/UUID/label; SWAPSPACE2 occupies the last ten page bytes.
        std::vector<char> header(page);
        const std::uint32_t version = 1, last_page = bytes / page - 1;
        std::memcpy(header.data() + 1024, &version, sizeof(version));
        std::memcpy(header.data() + 1028, &last_page, sizeof(last_page));
        std::memcpy(header.data() + page - 10, "SWAPSPACE2", 10);
        require(::pwrite(file.fd, header.data(), header.size(), 0) == page && ::fsync(file.fd) == 0, "Header write failed");
        std::cout << "Private swap: " << file.path << " bytes=" << bytes << " disk=" << *disk << std::endl;
        file.attempted = true;
        const auto activated = ::swapon(file.path.c_str(), 0);
        const auto activation_error = errno;
        if (activated != 0) throw std::runtime_error("swapon rejected errno=" + std::to_string(activation_error));
        file.active = true;
        const auto during = inventory(); preserved(before, during);
        const auto entry = std::find_if(during.begin(), during.end(), [&](const auto& e) { return e.path == file.path; });
        require(entry != during.end() && entry->file && entry->size_kib == (bytes - page) / 1024, "Active file inventory mismatch");
        auto mapped = read_linux_swap_path_disk(*entry);
        require(mapped.value == disk && mapped.support == Support::estimated, "Native pathname mapping failed");
        auto report = read_native_linux_swap_disks();
        require(report.value && report.value->disks.contains(*disk), "Native report missing active file disk");
        Monitor monitor; SampleOptions options;
        options.include_processes = options.include_networks = options.include_gpus = false;
        const auto snapshot = monitor.sample(options);
        auto found = std::find_if(snapshot.disks.begin(), snapshot.disks.end(), [&](const auto& d) { return d.id == *disk; });
        require(found != snapshot.disks.end() && found->contains_page_file.value == true &&
                found->contains_page_file.support == Support::estimated, "Public Monitor positive missing");
        std::cout << "PASS real /proc/swaps -> native file mapping -> report -> public Monitor; size_kib="
                  << entry->size_kib << " used_kib=" << entry->used_kib << '\n';
        require(file.cleanup(), "Cleanup incomplete; private file retained");
        const auto after = inventory(); preserved(before, after);
        require(!contains(after, file.path), "Private area still active");
        std::cout << "PASS swapoff + unlink + rmdir; existing swap configuration preserved\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL " << e.what() << '\n';
        if (file.cleanup()) std::cerr << "Private test resources cleaned\n";
        else std::cerr << "ATTENTION retained path=" << file.path << '\n';
        return 1;
    }
}
