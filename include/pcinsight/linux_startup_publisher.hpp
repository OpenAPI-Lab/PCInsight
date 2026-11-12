#pragma once
#include "types.hpp"
namespace pcinsight {
struct LinuxStartupPublisher {
    std::string desktop_file_id, component_id, developer_label, metadata_file;
};
// Explicit read-only AppStream upstream metadata scan in the caller's XDG data
// context. Optional system libxml2, no helpers, downloads or command execution.
// Labels are self-reported developers, NOT verified publishers/signers.
Metric<std::vector<LinuxStartupPublisher>> read_linux_startup_publishers();
// Exact desktop basename join only. Ambiguous/missing matches clear prior labels.
// Does not inspect Exec, infer from username, or claim executable ownership.
void apply_linux_startup_publishers(std::vector<StartupEntry>&,
    const Metric<std::vector<LinuxStartupPublisher>>&);
}
