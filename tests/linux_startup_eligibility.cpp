#include "../src/linux_startup_eligibility.hpp"
#include "../src/linux_startup_inventory.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#ifdef __linux__
#include <sys/stat.h>
#include <unistd.h>
#endif
using namespace pcinsight;
using namespace pcinsight::detail;
const std::string base = "[Desktop Entry]\nType=Application\nName=Demo\nExec=/demo\n";
int main() {
    XdgEligibilityContext context; context.desktop = "GNOME:Unity";
    std::string tested;
    context.executable = [&](const std::string& name) { tested = name; return Metric<bool>{true, Support::available, "injected file check"}; };
    auto check = [&](const std::string& conditions) { return xdg_standard_eligibility(base + conditions, context); };
    assert(check("").value == true);
    assert(check("X-GNOME-Autostart-enabled=false\n").value == true); // extension flag is not a universal XDG exclusion
    assert(check("OnlyShowIn=GNOME;KDE;\n").value == true);
    assert(check("OnlyShowIn=KDE;\n").value == false);
    assert(check("NotShowIn=Unity;\n").value == false);
    assert(check("NotShowIn=KDE;\n").value == true);
    assert(check("OnlyShowIn=gnome;\n").value == false); // case-sensitive
    assert(check("OnlyShowIn=\n").value == false);
    assert(check("TryExec=\n").value == true);
    assert(!check("OnlyShowIn=GNOME;\nNotShowIn=KDE;\n").value);
    assert(!check("OnlyShowIn=GNOME;\nOnlyShowIn=KDE;\n").value);
    assert(!check("OnlyShowIn=GNOME;;\n").value);
    assert(!check("TryExec=trailing\\\n").value);
    assert(check("TryExec=my\\sapp\n").value == true && tested == "my app");
    assert(!check("TryExec=bad\\qescape\n").value);
    assert(check("TryExec=literal;name\n").value == true && tested == "literal;name"); // never shell syntax
    assert(check("OnlyShowIn=GNOME;\n[Desktop Action Other]\nNotShowIn=GNOME;\nTryExec=other\n").value == true);
    context.desktop = "A;B";
    assert(check("OnlyShowIn=A\\;B;\n").value == true);
    context.desktop.reset();
    assert(!check("OnlyShowIn=GNOME;\n").value);
    assert(check("Hidden=true\nOnlyShowIn=GNOME;\n").value == false);
    context.executable = [](const std::string&) { return Metric<bool>{false, Support::available, "not executable"}; };
    assert(check("OnlyShowIn=GNOME;\nTryExec=missing\n").value == false); // known false dominates missing desktop
    context.executable = [](const std::string&) { return Metric<bool>{std::nullopt, Support::permission_required, "unavailable"}; };
    assert(check("TryExec=missing\n").support == Support::permission_required && !check("TryExec=missing\n").value);
    assert(!xdg_standard_eligibility("[Desktop Entry]\nType=Application\n", context).value);
    assert(xdg_standard_eligibility("[Desktop Entry]\nType=Directory\n", context).value == false);
#ifdef __linux__
    namespace fs = std::filesystem;
    const auto root = fs::temp_directory_path() / ("pcinsight-eligible-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    assert(fs::create_directory(root));
    struct Cleanup { fs::path path; ~Cleanup() { std::error_code ec; fs::remove_all(path, ec); } } cleanup{root};
    const auto executable = root / "my app";
    { std::ofstream f(executable); f << "not executed by the test\n"; assert(f); }
    assert(chmod(executable.c_str(), 0700) == 0);
    const auto* raw_path = std::getenv("PATH"); const std::optional<std::string> old_path = raw_path ? std::optional<std::string>(raw_path) : std::nullopt;
    struct Restore { std::optional<std::string> path; ~Restore() { if (path) setenv("PATH", path->c_str(), 1); else unsetenv("PATH"); } } restore{old_path};
    assert(setenv("PATH", root.c_str(), 1) == 0);
    auto native = native_xdg_eligibility_context(); native.desktop = "GNOME";
    assert(xdg_standard_eligibility(base + "TryExec=my\\sapp\n", native).value == true);
    assert(xdg_standard_eligibility(base + "TryExec=" + executable.string() + "\n", native).value == true);
    assert(xdg_standard_eligibility(base + "TryExec=missing\n", native).value == false);
    assert(!xdg_standard_eligibility(base + "TryExec=./my app\n", native).value);
    assert(xdg_standard_eligibility(base + "TryExec=" + root.string() + "\n", native).value == false);
    assert(chmod(executable.c_str(), 0600) == 0);
    assert(xdg_standard_eligibility(base + "TryExec=my\\sapp\n", native).value == false);
    assert(chmod(executable.c_str(), 0700) == 0);
    const auto autostart = root / "autostart"; fs::create_directory(autostart);
    { std::ofstream f(autostart / "example.desktop"); f << base << "OnlyShowIn=KDE;\n"; assert(f); }
    const auto inventory = read_xdg_startup_inventory({{autostart, true}}, 1048576, &native);
    assert(inventory.entries.size() == 1 && inventory.entries[0].status == StartupStatus::enabled);
    assert(inventory.entries[0].xdg_standard_eligible.value == false);
    struct DesktopEnvironment {
        std::vector<std::pair<std::string, std::optional<std::string>>> saved;
        DesktopEnvironment() {
            for (const auto* name : {"XDG_CONFIG_HOME", "XDG_CONFIG_DIRS", "XDG_CURRENT_DESKTOP"}) {
                const auto* value = std::getenv(name);
                saved.push_back({name, value ? std::optional<std::string>(value) : std::nullopt});
            }
        }
        ~DesktopEnvironment() {
            for (const auto& [name, value] : saved) {
                if (value) setenv(name.c_str(), value->c_str(), 1); else unsetenv(name.c_str());
            }
        }
    } environment;
    assert(setenv("XDG_CONFIG_HOME", root.c_str(), 1) == 0);
    assert(setenv("XDG_CONFIG_DIRS", root.c_str(), 1) == 0);
    assert(setenv("XDG_CURRENT_DESKTOP", "GNOME", 1) == 0);
    Monitor monitor; SampleOptions options;
    options.include_processes = options.include_disks = options.include_networks = options.include_gpus = false;
    options.include_startup_entries = true;
    const auto snapshot = monitor.sample(options);
    assert(snapshot.startup_entries.size() == 1 && snapshot.startup_entries[0].status == StartupStatus::enabled);
    assert(snapshot.startup_entries[0].xdg_standard_eligible.value == false);
    std::cout << "Native TryExec checks verified without executing the candidate; configured enabled and desktop-excluded kept separate\n";
#endif
}
