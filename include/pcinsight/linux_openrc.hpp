#pragma once
#include "metric.hpp"
#include <cstdint>
#include <vector>
namespace pcinsight {
// Filesystem marks, NOT process liveness or the librc service-state API.
enum class LinuxOpenrcMarkedState { started,stopping,starting,inactive };
// Our mask, NOT RC_SERVICE ABI bits: started1/stopping2/starting4/inactive8/
// failed16/hotplugged32/wasinactive64. No scheduled/crashed/stop inference.
Metric<LinuxOpenrcMarkedState> decode_linux_openrc_markers(std::uint32_t mask);
struct LinuxOpenrcMarkerEntry {
    std::string name;
    std::uint32_t marker_mask{}; // Symlink entries observed, targets NOT followed.
    Metric<LinuxOpenrcMarkedState> marked_state;
};
struct LinuxOpenrcMarkerInventory {
    std::string root;
    bool complete{true}; // Bounded census of the seven marker categories only.
    std::vector<LinuxOpenrcMarkerEntry> entries;
    std::vector<std::string> diagnostics;
};
// Explicit caller-trusted OpenRC runtime root (usually /run/openrc).
// Read-only; no mkdir/librc/CLI/scripts. Empty is NOT no installed services.
// Host serializes fork/setns/credential changes. No atomic/ABA guarantee.
Metric<LinuxOpenrcMarkerInventory> query_linux_openrc_markers(const std::string& absolute_root,std::uint32_t max_entries=1024);
struct LinuxOpenrcScriptEntry {
    std::string name;
    Metric<bool> executable_mode; // Any execute mode bit, NOT access/launch success.
};
enum class LinuxOpenrcReferenceKind { service,stacked_runlevel,unresolved };
struct LinuxOpenrcRunlevelReference {
    std::string name;
    LinuxOpenrcReferenceKind kind{LinuxOpenrcReferenceKind::unresolved};
    Metric<std::uint32_t> script_index; // Unique dev/inode match in THIS catalog.
    Metric<std::string> stacked_level; // Direct link only, no recursive expansion.
};
struct LinuxOpenrcRunlevelEntry {
    std::string name;
    std::vector<LinuxOpenrcRunlevelReference> references;
};
struct LinuxOpenrcCatalog {
    std::string init_directory,runlevel_directory;
    bool complete{true};
    std::vector<LinuxOpenrcScriptEntry> scripts;
    std::vector<LinuxOpenrcRunlevelEntry> runlevels;
    std::vector<std::string> diagnostics;
};
// One explicit init.d + runlevels scope, not automatic librc path precedence.
// File candidates/direct configured links, NOT effective activation or live state.
Metric<LinuxOpenrcCatalog> query_linux_openrc_catalog(const std::string& absolute_init_directory,const std::string& absolute_runlevel_directory,std::uint32_t max_scripts=1024);
}
