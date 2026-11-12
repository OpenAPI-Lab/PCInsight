#include "../src/linux_startup_publisher.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#ifdef __linux__
#include <sys/stat.h>
#include <unistd.h>
#endif
using namespace pcinsight;
using namespace pcinsight::detail;
namespace fs = std::filesystem;
namespace {
const std::string body = "<id>org.test.App</id><launchable type=\"desktop-id\">demo.desktop</launchable>";
std::string document(const std::string& label) {
    return "<?xml version=\"1.0\" encoding=\"UTF-8\"?><component type=\"desktop-application\">" + body + label + "</component>";
}
#ifdef __linux__
void write(const fs::path& path, const std::string& text) {
    std::ofstream stream(path, std::ios::binary); assert(stream); stream << text; stream.close(); assert(stream);
}
struct Environment {
    std::string key, old; bool existed{};
    Environment(const char* name, const std::string& value) : key(name) {
        const auto* original = std::getenv(name); existed = original != nullptr; if (original) old = original;
        assert(::setenv(name, value.c_str(), 1) == 0);
    }
    ~Environment() { if (existed) ::setenv(key.c_str(), old.c_str(), 1); else ::unsetenv(key.c_str()); }
};
#endif
}
int main(int argc, char** argv) {
#ifdef __linux__
    if (argc == 2 && std::string(argv[1]) == "--missing-reader") {
        assert(!read_linux_startup_publishers().value);
        const auto unavailable = parse_linux_startup_metainfo(document("<developer_name>Test</developer_name>"));
        assert(!unavailable.value && unavailable.support == Support::unsupported);
        std::cout << "PASS optional XML reader symbols unavailable: no metadata published\n";
        return 0;
    }
#else
    (void)argc; (void)argv;
#endif
    // Portable overlay: never infer publisher from command/name/source/username.
    std::vector<StartupEntry> entries(3);
    entries[0].id = "/private/demo.desktop"; entries[0].status = StartupStatus::enabled;
    entries[1].id = "/private/other.desktop"; entries[1].status = StartupStatus::disabled;
    entries[2].id = "/private/demo.desktop"; entries[2].status = StartupStatus::unknown;
    Metric<std::vector<LinuxStartupPublisher>> report{
        std::vector<LinuxStartupPublisher>{{"demo.desktop", "org.test.App", "Declared team", "/private/app.metainfo.xml"}},
        Support::estimated, "test"};
    apply_linux_startup_publishers(entries, report);
    assert(entries[0].publisher == "Declared team" && entries[0].publisher_label.value == "Declared team");
    assert(entries[0].publisher_label.support == Support::estimated && !entries[0].publisher_source_executable.value);
    assert(!entries[1].publisher_label.value && !entries[2].publisher_label.value);
    entries[0].status = StartupStatus::disabled;
    apply_linux_startup_publishers(entries, report); assert(entries[0].publisher_label.value == "Declared team");
    entries[0].id = "/private/Demo.desktop";
    apply_linux_startup_publishers(entries, report); assert(!entries[0].publisher_label.value);
    entries[0].id = "/private/demo.desktop";
    report.value->push_back(report.value->front());
    apply_linux_startup_publishers(entries, report); assert(entries[0].publisher.empty() && !entries[0].publisher_label.value);
    report.value->pop_back(); report.support = Support::unsupported;
    apply_linux_startup_publishers(entries, report); assert(entries[0].publisher.empty());
    report.support = Support::estimated; report.value->front().developer_label = "bad\nlabel";
    apply_linux_startup_publishers(entries, report); assert(!entries[0].publisher_label.value);
    report = {std::nullopt, Support::permission_required, "denied"};
    apply_linux_startup_publishers(entries, report); assert(!entries[0].publisher_label.value && entries[0].publisher_label.support == Support::permission_required);
    const auto defaults = resolve_linux_metainfo_directories("/home/test", "", "");
    assert(defaults.size() == 6 && defaults[0] == fs::path("/home/test/.local/share/metainfo"));
    const auto custom = resolve_linux_metainfo_directories("/home/test", "/custom", "/custom:relative::/system:/system");
    assert(custom.size() == 4 && custom[2] == fs::path("/system/metainfo"));
    try { (void)resolve_linux_metainfo_directories("", "", std::string(1048577, 'x')); assert(false); } catch (const std::invalid_argument&) {}
    assert(!parse_linux_startup_metainfo("<!DOCTYPE component SYSTEM 'file:///private/never-read'>").value);
    assert(!parse_linux_startup_metainfo(std::string(1048577, 'x')).value);
#ifdef __linux__
    const auto positive = parse_linux_startup_metainfo(document("<developer id=\"org.test\"><name>Team &amp; Co</name><name xml:lang=\"es\">Otro</name></developer>"));
    if (!positive.value && positive.note == "Optional system libxml2 reader unavailable") {
        std::cout << "Optional system XML reader absent: native XML positives NOT executed\n"; return 77;
    }
    assert(positive.value && positive.value->size() == 1 && positive.value->front().developer_label == "Team & Co");
    auto parsed = parse_linux_startup_metainfo(document("<developer_name>Legacy team</developer_name>"));
    assert(parsed.value && parsed.value->front().developer_label == "Legacy team");
    parsed = parse_linux_startup_metainfo(document("<developer xml:lang=\"fr\"><name>French only</name></developer>"));
    assert(parsed.value && parsed.value->empty());
    parsed = parse_linux_startup_metainfo(document("<developer><name>Equipo &#xF1;</name></developer>"));
    assert(parsed.value && parsed.value->front().developer_label == "Equipo \xC3\xB1");
    parsed = parse_linux_startup_metainfo(document("<developer_name><![CDATA[Team & Co]]></developer_name>"));
    assert(parsed.value && parsed.value->front().developer_label == "Team & Co");
    parsed = parse_linux_startup_metainfo(document("<developer_name>Same</developer_name><developer><name>Same</name></developer>"));
    assert(parsed.value && parsed.value->front().developer_label == "Same");
    for (const auto& bad : {
        document("<developer_name>One</developer_name><developer_name>Two</developer_name>"),
        document("<developer_name>One</developer_name><developer><name>Two</name></developer>"),
        document("<developer_name><b>Nested</b></developer_name>"),
        document("<developer_name>&missing;</developer_name>"),
        document("<developer_name>bad&#10;label</developer_name>"),
        document("<developer_name>Team</developer_name><id>duplicate</id>"),
        std::string("<component type=\"desktop-application\"><id>bad id</id><developer_name>Team</developer_name></component>"),
        document("<developer_name>Team</developer_name><launchable type=\"desktop-id\">../demo.desktop</launchable>"),
        document("<developer_name>Team</developer_name>") + "<extra/>",
        std::string("<components>") + document("<developer_name>Team</developer_name>") + "</components>",
        std::string("<!DOCTYPE component [<!ENTITY e SYSTEM 'file:///never-read'>]>") + document("<developer_name>&e;</developer_name>"),
        std::string("<component type=\"desktop-application\"><id>broken</component>")})
        assert(!parse_linux_startup_metainfo(bad).value);
    auto unrelated = parse_linux_startup_metainfo("<component type=\"font\"><id>font</id></component>");
    assert(unrelated.value && unrelated.value->empty());
    char pattern[] = "/tmp/pcinsight-publisher-XXXXXX";
    const auto* made = ::mkdtemp(pattern); assert(made);
    const fs::path root = made;
    struct Cleanup { fs::path path; ~Cleanup() { std::error_code ec; fs::remove_all(path, ec); } } cleanup{root};
    const auto data = root / "data", metadata = data / "metainfo", config = root / "config", system = root / "system";
    fs::create_directories(metadata); fs::create_directories(config / "autostart"); fs::create_directory(system);
    const auto source = metadata / "not-the-desktop-basename.metainfo.xml";
    write(source, document("<developer><name>Native test team</name></developer>"));
    write(config / "autostart/demo.desktop", "[Desktop Entry]\nType=Application\nName=Test\nExec=/never-executed\n");
    report = read_linux_startup_publishers_from({metadata});
    assert(report.value && report.value->size() == 1 && report.value->front().metadata_file == source.string());
    // Public native provider and public Monitor; only this process's environment changes.
    Environment data_home("XDG_DATA_HOME", data.string()), data_dirs("XDG_DATA_DIRS", system.string());
    Environment config_home("XDG_CONFIG_HOME", config.string()), config_dirs("XDG_CONFIG_DIRS", system.string());
    const auto native = read_linux_startup_publishers(); assert(native.value && native.value->size() == 1);
    Monitor monitor; SampleOptions options;
    options.include_processes = options.include_disks = options.include_networks = options.include_gpus = false;
    options.include_startup_entries = true;
    auto snapshot = monitor.sample(options); assert(snapshot.startup_entries.size() == 1 && !snapshot.startup_entries[0].publisher_label.value);
    options.include_linux_startup_publisher = true;
    snapshot = monitor.sample(options); assert(snapshot.startup_entries.size() == 1);
    assert(snapshot.startup_entries[0].publisher == "Native test team" && snapshot.startup_entries[0].publisher_label.support == Support::estimated);
    options.include_linux_startup_publisher = false;
    assert(monitor.sample(options).startup_entries[0].publisher.empty());
    options.include_linux_startup_publisher = true;
    write(metadata / "second.metainfo.xml", document("<developer_name>Conflicting team</developer_name>"));
    snapshot = monitor.sample(options); assert(snapshot.startup_entries[0].publisher.empty() && !snapshot.startup_entries[0].publisher_label.value);
    fs::remove(metadata / "second.metainfo.xml");
    write(metadata / "bad.metainfo.xml", "<broken>");
    report = read_linux_startup_publishers_from({metadata}); assert(!report.value);
    apply_linux_startup_publishers(snapshot.startup_entries, report); assert(!snapshot.startup_entries[0].publisher_label.value);
    fs::remove(metadata / "bad.metainfo.xml");
    fs::create_symlink(source, metadata / "link.metainfo.xml"); assert(!read_linux_startup_publishers_from({metadata}).value); fs::remove(metadata / "link.metainfo.xml");
    assert(::mkfifo((metadata / "fifo.metainfo.xml").c_str(), 0600) == 0);
    assert(!read_linux_startup_publishers_from({metadata}).value); fs::remove(metadata / "fifo.metainfo.xml");
    if (::geteuid() != 0) {
        assert(::chmod(source.c_str(), 0000) == 0); report = read_linux_startup_publishers_from({metadata});
        assert(::chmod(source.c_str(), 0600) == 0); assert(!report.value && report.support == Support::permission_required);
    }
    write(source, std::string(1048577, 'x')); assert(!read_linux_startup_publishers_from({metadata}).value);
    assert(!read_linux_startup_publishers_from({root / "data/../relative-not-directory", fs::path("relative")}).value);
    assert(!read_linux_startup_publishers_from(std::vector<fs::path>(129, metadata)).value);
    const auto missing = read_linux_startup_publishers_from({root / "absent"}); assert(missing.value && missing.value->empty());
    options.include_startup_entries = false;
    assert(monitor.sample(options).startup_entries.empty());
    std::cout << "PASS system libxml2 XML positives/negatives, private native files, public scan and Monitor opt-in, ambiguity, permissions and cleanup\n";
#else
    assert(!read_linux_startup_publishers().value);
    assert(!parse_linux_startup_metainfo(document("<developer_name>Test</developer_name>")).value);
    std::cout << "PASS portable join/context and non-Linux source absence; no native Linux XML claim\n";
#endif
}
