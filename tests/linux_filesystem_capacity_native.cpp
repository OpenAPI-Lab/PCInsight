#include "pcinsight/linux_filesystem_capacity.hpp"
#include <array>
#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/wait.h>
#include <unistd.h>
using namespace pcinsight;
namespace fs = std::filesystem;
namespace {
void require(bool ok, const char* reason) { if (!ok) throw std::runtime_error(reason); }
struct File { int fd{-1}; ~File() { if (fd >= 0) ::close(fd); } };
int root_block_read() {
    struct stat root{}; require(::stat("/", &root) == 0, "root stat failed");
    const auto identity = std::to_string(major(root.st_dev)) + ":" + std::to_string(minor(root.st_dev));
    std::ifstream event("/sys/dev/block/" + identity + "/uevent"); std::string line, name;
    while (std::getline(event, line)) if (line.starts_with("DEVNAME=")) name = line.substr(8);
    require(!name.empty() && name.size() < 256 && name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._-") == name.npos,
        "root has no simple direct device name; no raw-read fallback");
    LinuxFilesystemTarget target; target.path = "/dev/" + name;
    require(!read_linux_filesystem_capacity(target).value, "block read without explicit opt-in unexpectedly succeeded");
    target.allow_block_device = true;
    const auto result = read_linux_filesystem_capacity(target);
    if (!result.value) throw std::runtime_error(result.note);
    require(result.value->source_is_block_device && result.value->kernel_block_device.value == identity,
        "selected block identity differs from current root");
    require(result.value->declared_capacity_bytes <= result.value->source_size_bytes, "block extent exceeds source");
    std::cout << "PASS read-only root block metadata: device=" << identity << " declared_bytes=" << result.value->declared_capacity_bytes
              << " source_bytes=" << result.value->source_size_bytes << " checksum=" << result.value->superblock_checksum_valid.value.value_or(false)
              << " inventory_id=" << result.value->disk_inventory_id.value.value_or("unknown")
              << "; no filesystem/user content read beyond two primary superblocks\n";
    // Batch native positive only when the root is itself an unpartitioned
    // kernel disk: do not expand this manual read to unrelated partitions.
    if (result.value->disk_inventory_id.value) {
        const auto id = *result.value->disk_inventory_id.value;
        bool partitions = false;
        for (const auto& item : fs::directory_iterator("/sys/block/" + id)) if (fs::exists(item.path() / "partition")) partitions = true;
        if (!partitions) {
            require(!read_linux_disk_filesystem_capacity(id).value, "selected-disk opt-in guard failed");
            const auto batch = read_linux_disk_filesystem_capacity(id, true);
            if (!batch.value) throw std::runtime_error(batch.note);
            require(batch.value->regions.size() == 1 && batch.value->declared_total_bytes.value == result.value->declared_capacity_bytes,
                "native selected-disk total missing/mismatched");
            std::cout << "PASS selected-disk sysfs inventory -> guarded region probe -> complete logical declared total; no unrelated partitions read\n";
        }
    }
    return 0;
}
void format_private(const fs::path& path, const char* type) {
    // Only our exclusive, freshly created regular image. Never a block device.
    struct stat info{}; require(::lstat(path.c_str(), &info) == 0 && S_ISREG(info.st_mode) && info.st_nlink == 1,
        "formatter target is not private regular file");
    const auto child = ::fork(); require(child >= 0, "private formatter fork failed");
    if (!child) {
        ::execl(PCINSIGHT_TEST_MKE2FS, PCINSIGHT_TEST_MKE2FS, "-q", "-F", "-t", type, "-b", "4096", "-m", "0", "-E",
            "lazy_itable_init=0,lazy_journal_init=0", path.c_str(), static_cast<char*>(nullptr));
        ::_exit(127);
    }
    int status{}; pid_t reaped{};
    do { reaped = ::waitpid(child, &status, 0); } while (reaped < 0 && errno == EINTR);
    require(reaped == child && WIFEXITED(status) && WEXITSTATUS(status) == 0, "private image formatter failed");
}
}
int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string_view(argv[1]) == "--read-root-block-metadata") return root_block_read();
        require(argc == 1, "unexpected native test arguments");
        char pattern[] = "/tmp/pcinsight-filesystem-XXXXXX"; const auto* made = ::mkdtemp(pattern); require(made, "private image directory unavailable");
        const fs::path directory = made;
        struct Cleanup { fs::path directory; ~Cleanup() { std::error_code ec; fs::remove_all(directory, ec); } } cleanup{directory};
        constexpr std::uint64_t bytes = 32 * 1024 * 1024;
        LinuxFilesystemTarget target;
        for (const auto* type : {"ext2", "ext3", "ext4"}) {
            const auto path = directory / (std::string(type) + ".img");
            File file{::open(path.c_str(), O_CREAT | O_EXCL | O_NOFOLLOW | O_RDWR | O_CLOEXEC, 0600)};
            require(file.fd >= 0 && ::posix_fallocate(file.fd, 0, bytes) == 0, "private32MiB image allocation failed");
            format_private(path, type);
            target.path = path.string(); target.offset_bytes = 0; target.region_length_bytes.reset();
            target.expected_kernel_block_device = "8:99";
            require(!read_linux_filesystem_capacity(target).value, "image accepted as selected kernel block object");
            target.expected_kernel_block_device.reset();
            const auto result = read_linux_filesystem_capacity(target);
            if (!result.value) throw std::runtime_error(std::string(type) + ": " + result.note);
            require(result.support == Support::estimated && result.value->declared_capacity_bytes == bytes &&
                result.value->source_size_bytes == bytes && !result.value->source_is_block_device && !result.value->kernel_block_device.value,
                "wrong native unmounted image size/association");
            if (std::string_view(type) == "ext4") require(result.value->superblock_checksum_valid.value == true, "native ext4 checksum not validated");
            std::cout << "PASS native unmounted " << type << " image declared_bytes=" << result.value->declared_capacity_bytes << '\n';
            target.region_length_bytes = bytes - 4096; require(!read_linux_filesystem_capacity(target).value, "undersized selected region accepted");
            target.region_length_bytes.reset(); target.offset_bytes = UINT64_MAX; require(!read_linux_filesystem_capacity(target).value, "overflow offset accepted");
            target.offset_bytes = 0;
            fs::create_symlink(path, directory / "link.img"); target.path = (directory / "link.img").string();
            require(!read_linux_filesystem_capacity(target).value, "symlink target accepted"); fs::remove(directory / "link.img");
            if (::geteuid() != 0) {
                target.path = path.string(); require(::chmod(path.c_str(), 0000) == 0, "private permission setup failed");
                const auto denied = read_linux_filesystem_capacity(target);
                require(::chmod(path.c_str(), 0600) == 0, "private permission restore failed");
                require(!denied.value && denied.support == Support::permission_required, "native permission failure not preserved");
            }
        }
        // A real32MiB filesystem inside a larger48MiB regular source: do not
        // confuse source extent with formatted filesystem extent.
        fs::resize_file(directory / "ext4.img", bytes + 16 * 1024 * 1024);
        target.path = (directory / "ext4.img").string(); target.region_length_bytes.reset(); target.offset_bytes = 0;
        const auto larger = read_linux_filesystem_capacity(target);
        require(larger.value && larger.value->declared_capacity_bytes == bytes && larger.value->source_size_bytes == bytes + 16 * 1024 * 1024,
            "filesystem extent was replaced with larger source size");
        target.path = (directory / "fifo").string(); require(::mkfifo(target.path.c_str(), 0600) == 0, "private FIFO creation failed");
        require(!read_linux_filesystem_capacity(target).value, "FIFO metadata probe accepted");
        // Explicit subregion in a regular container, no partition-table inference.
        const auto source = directory / "ext4.img", container = directory / "region.img";
        File input{::open(source.c_str(), O_RDONLY | O_CLOEXEC)}, output{::open(container.c_str(), O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC, 0600)};
        require(input.fd >= 0 && output.fd >= 0 && ::ftruncate(output.fd, bytes + 8192) == 0, "private region fixture failed");
        std::array<char, 1024> header{};
        require(::pread(input.fd, header.data(), header.size(), 1024) == 1024 && ::pwrite(output.fd, header.data(), header.size(), 8192 + 1024) == 1024, "private region header copy failed");
        target.path = container.string(); target.offset_bytes = 8192; target.region_length_bytes = bytes;
        auto region = read_linux_filesystem_capacity(target);
        require(region.value && region.value->declared_capacity_bytes == bytes && region.value->region_offset_bytes == 8192,
            "explicit metadata region probe failed");
        // This container only copies a real header: NOT a complete second filesystem.
        header[0x80] ^= 1;
        require(::pwrite(output.fd, header.data(), header.size(), 8192 + 1024) == 1024, "checksum corruption fixture failed");
        require(!read_linux_filesystem_capacity(target).value, "corrupt metadata checksum accepted");
        std::cout << "PASS private native images/explicit region/checksum/permission/FIFO/symlink/range; no mounts or real disks changed\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "FAIL " << e.what() << '\n'; return 1; }
}
