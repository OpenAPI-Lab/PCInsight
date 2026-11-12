#pragma once
#include "pcinsight/metric.hpp"
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
namespace pcinsight::detail {
class PrivilegedRequestBudget {
public:
    using Clock=std::chrono::steady_clock;
    using Now=std::function<Clock::time_point()>;
    PrivilegedRequestBudget(std::uint64_t timeout_usec=5000000,std::uint32_t max_rpc=96,Now now={});
    bool live()const;
    std::uint32_t used()const;
    Metric<std::uint64_t> claim(std::uint64_t per_call_cap=5000000);
private:
    mutable std::mutex mutex_;
    Now now_;
    Clock::time_point start_{},deadline_{};
    std::uint32_t count_{},limit_{};
    bool valid_{};
};
// sd-bus calls using the DEFAULT timeout (including AddMatch) get a temporary
// remaining-time cap. Connection use MUST be serialized; restore on all exits.
template<class Bus>class PrivilegedRpcTimeout {
public:
    using Get=int(*)(Bus*,std::uint64_t*);using Set=int(*)(Bus*,std::uint64_t);
    PrivilegedRpcTimeout(Bus* bus,Get get,Set set,const std::shared_ptr<PrivilegedRequestBudget>& budget):bus_(bus),set_(set){
        if(!budget)return;
        if(!get||!set||get(bus,&old_)<0||!old_)throw std::runtime_error("No bounded default RPC timeout");
        const auto timeout=budget->claim(old_>5000000?5000000:old_);
        if(!timeout.value)throw std::runtime_error(timeout.note);
        if(set(bus,*timeout.value)<0)throw std::runtime_error("Cannot set remaining RPC timeout");
        restore_=true;
    }
    ~PrivilegedRpcTimeout(){if(restore_)set_(bus_,old_);}
    PrivilegedRpcTimeout(const PrivilegedRpcTimeout&)=delete;
private:
    Bus* bus_{};Set set_{};std::uint64_t old_{};bool restore_{};
};
std::uint64_t privileged_rpc_timeout(const std::shared_ptr<PrivilegedRequestBudget>&,std::uint64_t cap=5000000);
}
