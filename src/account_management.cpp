#include "pcinsight/account_management.hpp"
#include <utility>
namespace pcinsight {
AccountManagementIntegration::AccountManagementIntegration(Query query,Open open)
    :query_(std::move(query)),open_(std::move(open)){}
Metric<bool> AccountManagementIntegration::capability() const{
    if(!query_||!open_)return {std::nullopt,Support::unsupported,"account-management host integration not configured"};
    try{
        auto value=query_();
        if(value.note.size()>4096||value.note.find('\0')!=std::string::npos)
            return {std::nullopt,Support::unsupported,"invalid account-management host capability note"};
        if(value.support==Support::permission_required){value.value.reset();return value;}
        if(value.support!=Support::available||!value.value)
            return {std::nullopt,Support::unsupported,"host did not report a verified account-management capability"};
        return value;
    }catch(...){return {std::nullopt,Support::unsupported,"account-management capability callback failed"};}
}
AccountManagementOpenResult AccountManagementIntegration::request_open() const{
    const auto supported=capability();
    if(supported.support==Support::permission_required)
        return {{ActionError::permission_denied,supported.note},false};
    if(!supported.value||!*supported.value)
        return {{ActionError::not_supported,supported.note.empty()?"account-management UI unavailable":supported.note},false};
    try{
        return {open_(),true};
    }catch(...){
        return {{ActionError::system_error,"account-management host opener failed; UI may already have opened"},true};
    }
}
}
