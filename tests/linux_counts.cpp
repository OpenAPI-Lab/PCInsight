#include "../src/linux_counts.hpp"
#include <cassert>
#include <chrono>
#include <fstream>
#include <limits>
#ifndef _WIN32
#include <unistd.h>
#endif

namespace fs = std::filesystem;
using namespace pcinsight;
void write(const fs::path& path, const std::string& text) {
    std::ofstream f(path, std::ios::binary);
    assert(f);
    f << text;
    f.close();
    assert(f);
}
int main() {
    const auto root = fs::temp_directory_path() /
        ("pcinsight-counts-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    assert(fs::create_directory(root));
    struct Cleanup { fs::path path; ~Cleanup() { std::error_code ec; fs::remove_all(path, ec); } } cleanup{root};
    CpuInfo c;
    detail::read_linux_counts(c, root / "absent");
    assert(!c.processes.value && !c.threads.value && !c.handles.value);
    detail::read_linux_counts(c, root);
    assert(c.processes.value == 0 && c.threads.value == 0 && c.handles.value == 0);
    assert(c.processes.support == Support::available);
    write(root / "stat", "cpu 1 2 3 4\nprocesses 9999999\nprocs_running 9\n");
    fs::create_directories(root / "12" / "fd");
    fs::create_directories(root / "34" / "fd");
    fs::create_directory(root / "self");
    fs::create_directory(root / "123x");
    write(root / "999", "not a directory");
    write(root / "12" / "status", "Name:\tx\nThreads:\t2\n");
    write(root / "34" / "status", "Threads: 3\r\n");
    write(root / "12" / "fd" / "0", "");
    write(root / "12" / "fd" / "7", "");
    write(root / "12" / "fd" / "junk", "");
    detail::read_linux_counts(c, root);
    assert(c.processes.value == 2 && c.threads.value == 5 && c.handles.value == 2);
    assert(c.threads.support == Support::available && c.handles.support == Support::estimated);
    write(root / "34" / "status", "Threads: -1\n");
    fs::remove(root / "34" / "fd");
    detail::read_linux_counts(c, root);
    assert(c.threads.value == 2 && c.threads.support == Support::estimated);
    assert(c.handles.value == 2 && c.handles.support == Support::estimated);
    fs::remove_all(root / "12" / "fd");
    detail::read_linux_counts(c, root);
    assert(!c.handles.value && c.handles.support == Support::unsupported);
    fs::create_directory(root / "12" / "fd");
    detail::read_linux_counts(c, root);
    assert(c.handles.value == 0 && c.handles.support == Support::estimated);
    for (const auto& invalid : {"Threads: 0\n", "Threads: 1x\n", "Threads: 1\nThreads: 2\n",
             "Threads: 18446744073709551616\n", "Name: no threads\n"}) {
        write(root / "12" / "status", invalid);
        detail::read_linux_counts(c, root);
        assert(!c.threads.value && c.threads.support == Support::unsupported);
    }
    write(root / "12" / "status", std::string(65536, 'x'));
    detail::read_linux_counts(c, root);
    assert(!c.threads.value);
    write(root / "12" / "status", "Threads: 18446744073709551615\n");
    write(root / "34" / "status", "Threads: 1\n");
    detail::read_linux_counts(c, root);
    assert(!c.threads.value); // overflow is not a saturated, apparently valid count
#ifndef _WIN32
    if (geteuid() != 0) {
        fs::permissions(root, fs::perms::none);
        detail::read_linux_counts(c, root);
        fs::permissions(root, fs::perms::owner_all);
        assert(!c.processes.value && c.processes.support == Support::permission_required);
        fs::permissions(root / "12", fs::perms::none);
        detail::read_linux_counts(c, root);
        fs::permissions(root / "12", fs::perms::owner_all);
        assert(c.processes.value == 2 && c.threads.value == 1);
        assert(c.threads.support == Support::estimated);
    }
#endif
}
