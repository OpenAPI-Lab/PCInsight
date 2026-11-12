#include "linux_privileged_admission.hpp"
#include <algorithm>
namespace pcinsight::detail {
bool valid_privileged_admission_config(const PrivilegedAdmissionConfig& c){return c.global_burst&&c.global_burst<=512&&c.sender_burst&&c.sender_burst<=512&&c.global_per_second&&c.global_per_second<=1024&&c.sender_per_second&&c.sender_per_second<=1024&&c.max_senders&&c.max_senders<=256&&c.idle_timeout_usec>=1000000&&c.idle_timeout_usec<=300000000&&c.idle_timeout_usec>=(std::uint64_t(c.sender_burst)*1000000+c.sender_per_second-1)/c.sender_per_second&&c.max_read_queue&&c.max_read_queue<=1024&&c.max_write_queue&&c.max_write_queue<=1024;}
namespace {bool sender_name(std::string_view s){const auto dot=s.find('.',1);if(s.size()<4||s.size()>255||s[0]!=':'||dot==s.npos||dot==1||dot+1==s.size())return false;for(std::size_t i=1;i<s.size();++i)if(i!=dot&&(s[i]<'0'||s[i]>'9'))return false;return true;}}
PrivilegedAdmission::PrivilegedAdmission(PrivilegedAdmissionConfig config,Now now):config_(config),now_(now?std::move(now):Now{[]{return Clock::now();}}),valid_(valid_privileged_admission_config(config)){
    if(valid_)try{const auto now_value=now_();global_={std::uint64_t(config.global_burst)*1000000,now_value,now_value};}catch(...){valid_=false;}
}
void PrivilegedAdmission::refill(Bucket& b,std::uint32_t capacity,std::uint32_t rate,Clock::time_point now){const auto elapsed=std::chrono::duration_cast<std::chrono::microseconds>(now-b.updated).count();if(elapsed<=0)return;const auto capped=std::min<std::uint64_t>(std::uint64_t(elapsed),512000000);b.credit=std::min<std::uint64_t>(std::uint64_t(capacity)*1000000,b.credit+capped*rate);b.updated+=std::chrono::microseconds(elapsed);}
Metric<bool> PrivilegedAdmission::admit(std::string_view sender,std::uint64_t read,std::uint64_t write){std::lock_guard lock(mutex_);
    auto no=[](std::string note)->Metric<bool>{return {false,Support::available,std::move(note)};};
    if(!valid_||!sender_name(sender))return {{},Support::unsupported,"Admission configuration/sender invalid"};
    try{const auto now=now_();if(now<global_.seen){valid_=false;return {{},Support::unsupported,"Admission clock moved backwards; reconnect required"};}global_.seen=now;
        if(read>config_.max_read_queue||write>config_.max_write_queue)return no("Admission rejected local read/write queue pressure");
        refill(global_,config_.global_burst,config_.global_per_second,now);
        if(global_.credit<1000000)return no("Admission receiver-wide token limit");
        global_.credit-=1000000;
        std::erase_if(senders_,[&](const auto& item){return now>=item.second.seen&&now-item.second.seen>=std::chrono::microseconds(config_.idle_timeout_usec);});
        auto found=senders_.find(sender);if(found==senders_.end()){if(senders_.size()>=config_.max_senders)return no("Admission bounded sender table full; no active eviction");found=senders_.emplace(std::string(sender),Bucket{std::uint64_t(config_.sender_burst)*1000000,now,now}).first;}
        auto& bucket=found->second;refill(bucket,config_.sender_burst,config_.sender_per_second,now);bucket.seen=now;
        if(bucket.credit<1000000)return no("Admission sender token limit");
        bucket.credit-=1000000;
        return {true,Support::available,"Admitted before policy/identity/authorization RPC; not a permission grant"};
    }catch(...){valid_=false;return {{},Support::unsupported,"Admission clock/allocation failure; reconnect required"};}
}
std::size_t PrivilegedAdmission::tracked_senders()const{std::lock_guard lock(mutex_);return senders_.size();}
}
