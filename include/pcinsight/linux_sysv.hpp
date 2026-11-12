#pragma once
#include "metric.hpp"
#include <cstdint>
#include <vector>
#include <string_view>
namespace pcinsight {
enum class LinuxSysVLinkAction { start,stop };
struct LinuxSysVLinkConvention {
    LinuxSysVLinkAction action{};
    std::uint8_t sequence{}; // Filename NN only, NOT effective dependency order.
    std::string suffix; // Filename suffix, NOT a service identity/path to execute.
};
Metric<LinuxSysVLinkConvention> decode_linux_sysv_link_name(const std::string&);
struct LinuxSysVScriptCandidate {std::string name;Metric<bool> executable_mode;};
struct LinuxSysVRunlevelReference {
    std::string name;
    Metric<LinuxSysVLinkConvention> convention;
    Metric<bool> symlink,regular_target;
    Metric<std::uint32_t> script_index; // Unique dev/inode match, THIS result only.
};
struct LinuxSysVRunlevel {
    std::string name; // 0..9 or S; directory is rc{name}.d under supplied root.
    Metric<bool> directory_present;
    std::vector<LinuxSysVRunlevelReference> entries;
};
struct LinuxSysVCatalog {
    std::string init_directory,runlevel_root;
    bool complete{true};
    std::vector<LinuxSysVScriptCandidate> scripts;
    std::vector<LinuxSysVRunlevel> runlevels;
    std::vector<std::string> diagnostics;
};
// Explicit init.d and parent of rc[0-9S].d (e.g. /etc or /etc/rc.d).
// Read-only metadata. NO script bodies/exec/CLI/enable/disable/status/PID/liveness.
// max_entries counts scripts+runlevel entries, not the eleven fixed level rows.
Metric<LinuxSysVCatalog> query_linux_sysv_catalog(const std::string& absolute_init_directory,const std::string& absolute_runlevel_root,std::uint32_t max_entries=1024);
struct LinuxSysVLsbMetadata {
    Metric<std::string> short_description,description; // Declared comments only.
};
// Strict leading-comment LSB block, <=64KiB input, no shell/variable evaluation.
// input_complete=false means bounded prefix, not assumed EOF; code ends preamble.
Metric<LinuxSysVLsbMetadata> parse_linux_sysv_lsb_metadata(std::string_view,bool input_complete=true);
// Fresh explicit regular file observation, NOT a token from an earlier catalog.
Metric<LinuxSysVLsbMetadata> read_linux_sysv_lsb_metadata(const std::string& absolute_script_path);
struct LinuxSysVDescribedCatalog {
    LinuxSysVCatalog catalog;
    std::vector<Metric<LinuxSysVLsbMetadata>> metadata; // Aligned with catalog.scripts, SAME call only.
    bool all_metadata_reads_attempted{true}; // Not a promise that fields/header exist.
    std::uint64_t prefix_bytes_reserved{}; // Worst-case reservation, NOT measured disk I/O.
};
// Opt-in content read BETWEEN catalog scans, tied to candidate identity, not name join.
// Budget0 disables reads, max16MiB; each candidate reserves two <=64KiB prefixes.
Metric<LinuxSysVDescribedCatalog> query_linux_sysv_described_catalog(const std::string& absolute_init_directory,const std::string& absolute_runlevel_root,std::uint32_t max_entries=1024,std::uint64_t max_prefix_bytes=8*1024*1024);
}
