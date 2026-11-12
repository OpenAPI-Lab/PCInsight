#include "linux_privileged_budget.hpp"
#include <algorithm>
namespace pcinsight::detail {
PrivilegedRequestBudget::PrivilegedRequestBudget(std::uint64_t timeout,std::uint32_t limit,Now now):now_(now?std::move(now):Now{[]{return Clock::now();}}),limit_(limit){
    if(timeout<1000||timeout>30000000||!limit||limit>128)return;
    try{start_=now_();const auto delta=std::chrono::microseconds(timeout);if(start_>Clock::time_point::max()-delta)return;deadline_=start_+delta;valid_=true;}catch(...){valid_=false;}
}
bool PrivilegedRequestBudget::live()const{std::lock_guard lock(mutex_);try{const auto now=now_();return valid_&&now>=start_&&now<deadline_;}catch(...){return false;}}
std::uint32_t PrivilegedRequestBudget::used()const{std::lock_guard lock(mutex_);return count_;}
Metric<std::uint64_t> PrivilegedRequestBudget::claim(std::uint64_t cap){std::lock_guard lock(mutex_);
    auto no=[]()->Metric<std::uint64_t>{return {{},Support::unsupported,"Request RPC budget/deadline invalid or exhausted; no further call"};};
    try{const auto now=now_();if(!valid_||!cap||now<start_||now>=deadline_||count_>=limit_)return no();const auto remaining=std::chrono::duration_cast<std::chrono::microseconds>(deadline_-now).count();if(remaining<=0)return no();++count_;return {std::min<std::uint64_t>(std::min<std::uint64_t>(cap,5000000),std::uint64_t(remaining)),Support::available,"One RPC charged to shared request deadline"};}catch(...){return no();}
}
std::uint64_t privileged_rpc_timeout(const std::shared_ptr<PrivilegedRequestBudget>& budget,std::uint64_t cap){if(!budget)return cap;const auto next=budget->claim(cap);if(!next.value)throw std::runtime_error(next.note);return *next.value;}
}
