#pragma once
#include "actions.hpp"
#include "types.hpp"
namespace pcinsight {
// Payload types are in types.hpp for use in snapshots as well; layouts unchanged.
// Local-drive user/common Startup selected by the inventory ID prefix; explicit registration management,
// NOT Windows approval, execution eligibility, or authenticated backup authorship.
// Reads both the direct file and PCInsight.Disabled.v1/<same name>, without writes.
Metric<WindowsStartupFileRegistration> read_windows_startup_file_registration(const std::string& entry_id);
// Discover direct regular files in PCInsight.Disabled.v1 after application restart.
// Includes conflicts with live files. Bounded, read-only, two matching observations,
// estimated/non-atomic; no claim of authenticated authorship or Windows approval.
Metric<std::vector<WindowsStartupFileBackup>> list_windows_startup_file_backups();
// Separate explicit common-folder discovery; may affect ALL users if restored.
// Does not create folders, elevate, or silently extend the current-user listing above.
Metric<std::vector<WindowsStartupFileBackup>> list_windows_common_startup_file_backups();
// false: rename into that subdirectory; true: restore. One no-replace rename
// of the checked file handle. Pass the latest observation unchanged. No shell,
// elevation, deletion, cross-volume copy, or implicit fallback from other APIs.
// The host must obtain consent; the free function does not provide audit logging.
// A winfolder:common: ID selects FOLDERID_CommonStartup, with caller's existing rights;
// show the all-users impact explicitly and never retry elevated automatically.
ActionResult set_windows_startup_file_registered(const WindowsStartupFileRegistration& observed, bool registered);
}
