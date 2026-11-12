#pragma once
#include "linux_privileged_identity.hpp"
#include "linux_privileged_jobs.hpp"
#include "linux_privileged_admission.hpp"
#include <memory>
namespace pcinsight::detail {
// INTERNAL process-owned objects, NOT proof of trusted loader/process bootstrap.
class PrivilegedDirectory {
public:
    struct Impl;
    ~PrivilegedDirectory();
    int native_fd()const; // Borrowed; no close/offset mutation by caller.
    std::string absolute_path()const;
    bool revalidate()const;
private:
    explicit PrivilegedDirectory(std::unique_ptr<Impl>);
    std::unique_ptr<Impl> impl_;
    friend Metric<std::shared_ptr<PrivilegedDirectory>> open_privileged_directory(std::string,std::uint32_t,bool);
    friend class PrivilegedBusConnection;
};
// Absolute lexical path; each component pinned without symlinks. Root or owner
// may own ancestors, final directory MUST belong to owner. No sticky exception
// by default; tests explicitly permit sticky ancestors (never the final dir).
Metric<std::shared_ptr<PrivilegedDirectory>> open_privileged_directory(std::string path,std::uint32_t owner_uid=0,bool allow_sticky_ancestors=false);
struct PrivilegedBusEndpoint {
    std::shared_ptr<PrivilegedDirectory> directory;
    std::string socket_name;
    std::uint32_t socket_owner_uid{},bus_peer_uid{}; // Separate: file owner != daemon UID.
    std::uint64_t call_timeout_usec{2000000}; // Nonzero <=5 seconds, not total deadline.
    PrivilegedAdmissionConfig admission;
};
class PrivilegedBusConnection {
public:
    struct Impl;
    ~PrivilegedBusConnection();
    void* native_handle()const; // Serialized use only; never retain after disconnect.
    PrivilegedIdentityConfig identity_config(std::uint32_t authority_uid)const;
    std::uint64_t generation()const;
    void disconnect(); // No flush, execution, reconnect or service activation.
    Metric<bool> admit_request(void* incoming_message)const;
    Metric<std::shared_ptr<PrivilegedNativeJobWatch>> prepare_job_watch(std::string manager_owner,std::string unit,std::uint64_t timeout_usec,std::shared_ptr<PrivilegedRequestBudget> budget={})const;
private:
    explicit PrivilegedBusConnection(std::unique_ptr<Impl>);
    std::unique_ptr<Impl> impl_;
    friend Metric<std::shared_ptr<PrivilegedBusConnection>> connect_privileged_bus(PrivilegedBusEndpoint);
};
Metric<std::shared_ptr<PrivilegedBusConnection>> connect_privileged_bus(PrivilegedBusEndpoint);
PrivilegedAuthorizationObservation receive_owned_privileged_request(void*,const PrivilegedBusConnection&,std::uint32_t authority_uid,const PrivilegedPolicyStore&,bool permit_interaction=false);
}
