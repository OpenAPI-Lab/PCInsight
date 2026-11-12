#include "../src/linux_startup_inventory.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#ifdef __linux__
#include <sys/stat.h>
#include <unistd.h>
#endif
using namespace pcinsight;
using namespace pcinsight::detail;
namespace fs = std::filesystem;
void write(const fs::path& path, const std::string& value) { std::ofstream f(path, std::ios::binary); assert(f); f << value; f.close(); assert(f); }
int main() {
    auto defaults = resolve_xdg_autostart("/home/demo", "", "");
    assert(defaults.size() == 2 && defaults[0].personal && !defaults[1].personal);
    assert(defaults[0].path == fs::path("/home/demo/.config/autostart") && defaults[1].path == fs::path("/etc/xdg/autostart"));
    auto custom = resolve_xdg_autostart("/home/demo", "/custom", "/first:relative::/second:/first:/custom");
    assert(custom.size() == 3 && custom[0].path == fs::path("/custom/autostart"));
    assert(custom[1].path == fs::path("/first/autostart") && custom[2].path == fs::path("/second/autostart"));
    assert(resolve_xdg_autostart("/home/demo", "relative", "relative")[0].path == defaults[0].path);
    assert(resolve_xdg_autostart("", "", "").size() == 1);
    std::string too_many;
    for (unsigned i = 0; i < 66; ++i) too_many += "/directory" + std::to_string(i) + ":";
    try { (void)resolve_xdg_autostart("/home/demo", "", too_many); assert(false); } catch (const std::invalid_argument&) {}
    const auto root = fs::temp_directory_path() / ("pcinsight-xdg-inventory-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    assert(fs::create_directory(root));
    struct Cleanup { fs::path root; ~Cleanup() { std::error_code ec; fs::remove_all(root, ec); } } cleanup{root};
    const auto user = root / "user", first = root / "first", second = root / "second";
    fs::create_directory(user); fs::create_directory(first); fs::create_directory(second);
    const std::string enabled = "[Desktop Entry]\nType=Application\nName=System app\nExec=/system/app\n";
    write(first / "same.desktop", enabled); write(second / "same.desktop", enabled);
    write(user / "same.desktop", "[Desktop Entry]\nHidden=true\n");
    write(first / "priority.desktop", "[Desktop Entry]\nName=First\nExec=/first\n");
    write(second / "priority.desktop", "[Desktop Entry]\nName=Second\nExec=/second\n");
    write(second / "fallback.desktop", enabled);
    write(user / "broken.desktop", "invalid"); write(first / "broken.desktop", enabled);
    write(user / "ignored.txt", enabled);
    fs::create_directory(user / "directory.desktop");
    write(first / "directory.desktop", enabled);
    std::vector<XdgAutostartDirectory> dirs{{user, true}, {first, false}, {second, false}};
    auto inventory = read_xdg_startup_inventory(dirs);
    try { (void)read_xdg_startup_inventory(dirs, 1); assert(false); } catch (const std::length_error&) {}
    assert(inventory.complete && inventory.entries.size() == 5);
    std::map<std::string, StartupEntry> entries;
    for (const auto& row : inventory.entries) entries[fs::path(row.id).filename().string()] = row;
    assert(entries.at("same.desktop").id == (user / "same.desktop").string());
    assert(entries.at("same.desktop").status == StartupStatus::disabled); // hidden masks all system copies
    assert(entries.at("priority.desktop").name == "First" && entries.at("priority.desktop").command == "/first");
    assert(entries.at("broken.desktop").status == StartupStatus::unknown && entries.at("broken.desktop").id == (user / "broken.desktop").string());
    assert(entries.at("fallback.desktop").id == (second / "fallback.desktop").string());
    assert(entries.at("directory.desktop").id == (first / "directory.desktop").string());
    auto absent = read_xdg_startup_inventory({{root / "absent", true}, {first, false}});
    assert(absent.complete && absent.entries.size() == 4);
    write(root / "not-directory", "file");
    const auto inaccessible = read_xdg_startup_inventory({{root / "not-directory", true}, {first, false}});
    assert(!inaccessible.complete && !inaccessible.diagnostics.empty());
    for (const auto& row : inaccessible.entries) assert(row.status == StartupStatus::unknown);
#ifdef __linux__
    if (geteuid() != 0) {
        assert(chmod(user.c_str(), 0000) == 0);
        inventory = read_xdg_startup_inventory(dirs);
        assert(chmod(user.c_str(), 0700) == 0);
        assert(!inventory.complete && !inventory.diagnostics.empty());
        for (const auto& row : inventory.entries) assert(row.status == StartupStatus::unknown);
        assert(chmod((user / "same.desktop").c_str(), 0000) == 0);
        inventory = read_xdg_startup_inventory(dirs);
        assert(chmod((user / "same.desktop").c_str(), 0600) == 0);
        bool found = false;
        for (const auto& row : inventory.entries) if (fs::path(row.id).filename() == "same.desktop") {
            found = true; assert(row.id == (user / "same.desktop").string() && row.status == StartupStatus::unknown);
        }
        assert(found); // don't fall through to the readable lower-priority file
    }
    fs::create_symlink(root / "missing", user / "dangling.desktop"); write(first / "dangling.desktop", enabled);
    assert(mkfifo((user / "pipe.desktop").c_str(), 0600) == 0);
    inventory = read_xdg_startup_inventory(dirs);
    bool dangling = false;
    for (const auto& row : inventory.entries) {
        assert(fs::path(row.id).filename() != "pipe.desktop");
        if (fs::path(row.id).filename() == "dangling.desktop") { dangling = true; assert(row.status == StartupStatus::unknown); }
    }
    assert(dangling);
    Monitor monitor;
    SampleOptions options;
    options.include_processes = options.include_disks = options.include_networks = options.include_gpus = false;
    assert(monitor.sample(options).startup_entries.empty());
    options.include_startup_entries = true;
    const auto sample = monitor.sample(options);
    std::set<std::string> names;
    for (const auto& row : sample.startup_entries) assert(names.insert(fs::path(row.id).filename().string()).second);
    options.include_startup_entries = false;
    assert(monitor.sample(options).startup_entries.empty());
    std::cout << "Native caller-context startup inventory: " << sample.startup_entries.size() << " effective entries\n";
    const auto personal_config = root / "context-user", system_config = root / "context-system";
    fs::create_directories(personal_config / "autostart"); fs::create_directories(system_config / "autostart");
    write(personal_config / "autostart" / "context.desktop", "[Desktop Entry]\nHidden=true\n");
    write(system_config / "autostart" / "context.desktop", enabled);
    struct Environment {
        std::optional<std::string> personal, system;
        Environment() {
            if (auto* value = std::getenv("XDG_CONFIG_HOME")) personal = value;
            if (auto* value = std::getenv("XDG_CONFIG_DIRS")) system = value;
        }
        ~Environment() {
            if (personal) setenv("XDG_CONFIG_HOME", personal->c_str(), 1); else unsetenv("XDG_CONFIG_HOME");
            if (system) setenv("XDG_CONFIG_DIRS", system->c_str(), 1); else unsetenv("XDG_CONFIG_DIRS");
        }
    } environment; // only this single-threaded test process, never parent/OS settings
    assert(setenv("XDG_CONFIG_HOME", personal_config.c_str(), 1) == 0);
    assert(setenv("XDG_CONFIG_DIRS", system_config.c_str(), 1) == 0);
    options.include_startup_entries = true;
    const auto overridden = monitor.sample(options);
    assert(overridden.startup_entries.size() == 1);
    assert(overridden.startup_entries[0].id == (personal_config / "autostart" / "context.desktop").string());
    assert(overridden.startup_entries[0].status == StartupStatus::disabled);
    std::cout << "Monitor with private XDG context: personal Hidden override masks system copy\n";
#endif
}
