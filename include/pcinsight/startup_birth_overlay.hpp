#pragma once
#include "startup_store.hpp"
namespace pcinsight {
struct StartupBirthOverlayContext {
    std::string capture_id;
    std::array<std::uint64_t,2> boot{};
    std::uint64_t pid_namespace_device{},pid_namespace_inode{};
    bool accept_declared_associations{false}; // Explicit opt-in, not proof of startup/app identity.
};
struct StartupBirthOverlayResult {
    ActionError error{ActionError::none};
    std::size_t applied_entries{},not_present_observations{},unbound_observations{};
    std::string message;
    explicit operator bool() const {return error==ActionError::none;}
};
// Pure explicit HISTORICAL overlay of accepted capture context + declared entry
// IDs. Does not establish current snapshot boot/catalog/app identity, authenticate
// files, read OS, start collection, or certify automatic startup cause. Caller
// selects/trusts the source and must present scope/confidence. No name/PID matching.
// Invalid batches leave snapshot unchanged. Multiple groups are NEVER summed:
// CPU/disk/interval unknown, high lower bound survives if any group supports it.
// Legacy steady-clock interval remains unknown even for a single birth row.
StartupBirthOverlayResult apply_startup_birth_impact(SystemSnapshot&,
    const StartupBirthArchive&,const StartupBirthOverlayContext&);
}
