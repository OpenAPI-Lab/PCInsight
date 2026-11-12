#include "../src/linux_startup.hpp"
#include <cassert>
#include <chrono>
#include <cerrno>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <iostream>
#ifdef __linux__
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <fcntl.h>
#include <unistd.h>
#endif
using namespace pcinsight;
using namespace pcinsight::detail;
namespace fs = std::filesystem;
void write(const fs::path& p, const std::string& text) { std::ofstream f(p, std::ios::binary); assert(f); f << text; f.close(); assert(f); }
std::string read(const fs::path& p) { std::ifstream f(p, std::ios::binary); assert(f); return {std::istreambuf_iterator<char>(f), {}}; }
int main() {
    const std::string input = "# comment\r\n[Desktop Entry]\r\nType=Application\r\nName=Demo\r\nName[es]=Demostración\r\nExec=/demo --arg\r\nHidden=true\r\nActions=More;\r\n[Desktop Action More]\r\nHidden=true\r\nExec=/other";
    const auto enabled = rewrite_xdg_startup(input, true); assert(enabled);
    assert(enabled->find("Hidden=false\r\n") != std::string::npos);
    assert(enabled->find("X-GNOME-Autostart-enabled=true\r\n[Desktop Action More]\r\nHidden=true\r\nExec=/other") != std::string::npos);
    assert(enabled->find("Name[es]=Demostración\r\nExec=/demo --arg") != std::string::npos);
    assert(rewrite_xdg_startup(*enabled, true) == enabled);
    const auto parsed = read_xdg_startup(*enabled); assert(parsed);
    assert(parsed->status == StartupStatus::enabled && parsed->name == "Demo" && parsed->command == "/demo --arg");
    assert(read_xdg_startup(input)->status == StartupStatus::disabled);
    assert(!read_xdg_startup("[Desktop Entry]\nHidden=not-a-bool\n"));
    const auto disabled = rewrite_xdg_startup(*enabled, false); assert(disabled);
    assert(disabled->find("Hidden=true\r\n") != std::string::npos);
    assert(disabled->find("X-GNOME-Autostart-enabled=false\r\n[Desktop Action More]") != std::string::npos);
    const auto minimal = rewrite_xdg_startup("[Desktop Entry]", false);
    assert(minimal == "[Desktop Entry]\nHidden=true\nX-GNOME-Autostart-enabled=false\n");
    const auto spaces = rewrite_xdg_startup("[Desktop Entry]\n Hidden = false\nX-GNOME-Autostart-enabled = true", false);
    assert(spaces == "[Desktop Entry]\n Hidden =true\nX-GNOME-Autostart-enabled =false");
    for (const auto& bad : {"", "Name=No group", "[Other]\n[Desktop Entry]\n", "[Desktop Entry\n", "[Desktop Entry]\nHidden=true\nHidden=false\n",
        "[Desktop Entry]\nX-GNOME-Autostart-enabled=true\nX-GNOME-Autostart-enabled=false\n", "[Desktop Entry]\n[Desktop Entry]\n"})
        assert(!rewrite_xdg_startup(bad, true));
    assert(!rewrite_xdg_startup(std::string("[Desktop Entry]\n\0", 17), true));
    assert(!rewrite_xdg_startup(std::string(1048577, 'a'), true));
#ifdef __linux__
    const auto root = fs::temp_directory_path() / ("pcinsight-startup-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    assert(fs::create_directory(root));
    struct Cleanup { fs::path root; ~Cleanup() { std::error_code ec; fs::permissions(root, fs::perms::owner_all, ec); fs::remove_all(root, ec); } } cleanup{root};
    const auto path = root / "app.desktop";
    write(path, input);
    assert(chmod(path.c_str(), 0640) == 0);
    struct stat original{}; assert(stat(path.c_str(), &original) == 0);
    const bool attributes = setxattr(path.c_str(), "user.pcinsight", "keep", 4, 0) == 0;
    const auto decoy = root / (".pcinsight-startup-" + std::to_string(getpid()) + "-0");
    write(decoy, "unrelated temp sentinel");
    unsigned char acl[44]{};
    auto put = [&](unsigned offset, unsigned value, unsigned width) {
        for (unsigned i = 0; i < width; ++i) acl[offset + i] = static_cast<unsigned char>(value >> (8 * i));
    };
    put(0, 2, 4); // Linux POSIX ACL xattr version
    const unsigned tags[]{1, 2, 4, 16, 32}, perms[]{7, 4, 0, 4, 0};
    for (unsigned i = 0; i < 5; ++i) {
        put(4 + 8 * i, tags[i], 2); put(6 + 8 * i, perms[i], 2);
        put(8 + 8 * i, i == 1 ? 12345 : 0xffffffffu, 4);
    }
    const bool default_acl = setxattr(root.c_str(), "system.posix_acl_default", acl, sizeof(acl), 0) == 0;
    auto actions = make_system_actions();
    assert(actions->set_startup_enabled(path.string(), true));
    assert(read(path) == *enabled && read(decoy) == "unrelated temp sentinel");
    assert(read_xdg_startup(read(path))->status == StartupStatus::enabled);
    struct stat after{}; assert(stat(path.c_str(), &after) == 0);
    assert((after.st_mode & 07777) == 0640 && after.st_uid == original.st_uid && after.st_gid == original.st_gid);
    if (default_acl) {
        assert(getxattr(path.c_str(), "system.posix_acl_access", nullptr, 0) < 0 && errno == ENODATA);
        std::cout << "Extra inherited default ACL removed: verified\n";
    }
    if (attributes) { char value[4]; assert(getxattr(path.c_str(), "user.pcinsight", value, 4) == 4); assert(std::string(value, 4) == "keep"); }
    std::cout << "Linux startup fixtures: ownership/mode preserved; xattr=" << (attributes ? "verified" : "unsupported by fixture filesystem") << '\n';
    assert(actions->set_startup_enabled(path.string(), false)); assert(read(path) == *disabled);
    assert(read_xdg_startup(read(path))->status == StartupStatus::disabled);
    assert(actions->set_startup_enabled(path.string(), false)); assert(read(path) == *disabled);
    auto symlink = root / "link.desktop";
    fs::create_symlink(path, symlink);
    assert(actions->set_startup_enabled(symlink.string(), true).error == ActionError::invalid_argument);
    assert(read(path) == *disabled);
    auto hardlink = root / "hard.desktop"; fs::create_hard_link(path, hardlink);
    assert(actions->set_startup_enabled(path.string(), true).error == ActionError::invalid_argument);
    assert(read(hardlink) == *disabled); fs::remove(hardlink);
    auto bad = root / "bad.desktop"; write(bad, "[Desktop Entry]\nHidden=true\nHidden=false\n");
    const auto bad_before = read(bad);
    assert(actions->set_startup_enabled(bad.string(), true).error == ActionError::invalid_argument && read(bad) == bad_before);
    assert(actions->set_startup_enabled((root / "absent.desktop").string(), true).error == ActionError::not_found);
    int dir = open(root.c_str(), O_RDONLY | O_DIRECTORY); assert(dir >= 0);
    assert(flock(dir, LOCK_EX | LOCK_NB) == 0);
    assert(actions->set_startup_enabled(path.string(), true).error == ActionError::stale_target);
    assert(close(dir) == 0 && read(path) == *disabled);
    if (geteuid() != 0) {
        assert(chmod(root.c_str(), 0500) == 0);
        const auto denied = actions->set_startup_enabled(path.string(), true);
        assert(chmod(root.c_str(), 0700) == 0);
        assert(denied.error == ActionError::permission_denied && read(path) == *disabled);
    }
    for (const auto& file : fs::directory_iterator(root))
        if (file.path().filename().string().starts_with(".pcinsight-startup-")) assert(file.path() == decoy);
#endif
}
