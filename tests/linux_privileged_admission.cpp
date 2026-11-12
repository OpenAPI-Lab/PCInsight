#include "../src/linux_privileged_admission.hpp"
#include <cassert>
#include <atomic>
#include <thread>
#include <vector>
using namespace pcinsight::detail;
int main(){using Clock=PrivilegedAdmission::Clock;auto now=Clock::time_point{};PrivilegedAdmissionConfig c;c.global_burst=3;c.global_per_second=1;c.sender_burst=2;c.sender_per_second=1;c.idle_timeout_usec=2000000;
    PrivilegedAdmission a(c,[&]{return now;});assert(a.admit(":1.2").value==true&&a.admit(":1.2").value==true);assert(a.admit(":1.2").value==false&&a.admit(":1.3").value==false);assert(a.tracked_senders()==1);now+=std::chrono::seconds(1);assert(a.admit(":1.2").value==true);now+=std::chrono::seconds(2);assert(a.admit(":1.3").value==true&&a.tracked_senders()==1);
    c.global_burst=8;c.sender_burst=1;c.max_senders=2;c.idle_timeout_usec=1000000;now={};PrivilegedAdmission table(c,[&]{return now;});assert(table.admit(":1.1").value==true&&table.admit(":1.2").value==true&&table.admit(":1.3").value==false&&table.tracked_senders()==2);now+=std::chrono::seconds(1);assert(table.admit(":1.3").value==true&&table.tracked_senders()==1);
    c.global_burst=1;c.max_read_queue=c.max_write_queue=1;PrivilegedAdmission queues(c);assert(queues.admit(":1.1",2,0).value==false&&queues.admit(":1.1",0,2).value==false&&queues.admit(":1.1",1,1).value==true);
    c.global_burst=8;c.sender_per_second=2;now={};PrivilegedAdmission fractions(c,[&]{return now;});assert(fractions.admit(":1.1").value==true);now+=std::chrono::microseconds(250000);assert(fractions.admit(":1.1").value==false);now+=std::chrono::microseconds(250000);assert(fractions.admit(":1.1").value==true);
    for(const auto* sender:{"","org.pcinsight.Client",":1..2",":a.2",":1."})assert(!fractions.admit(sender).value);
    auto bad=c;bad.sender_per_second=0;assert(!valid_privileged_admission_config(bad));PrivilegedAdmission invalid(bad);assert(!invalid.admit(":1.1").value);bad=c;bad.sender_burst=16;assert(!valid_privileged_admission_config(bad));bad=c;bad.max_senders=257;assert(!valid_privileged_admission_config(bad));
    now={};PrivilegedAdmission backwards(c,[&]{return now;});now+=std::chrono::microseconds(1);assert(backwards.admit(":1.1").value==true);now-=std::chrono::nanoseconds(1);assert(!backwards.admit(":1.1").value);now+=std::chrono::seconds(1);assert(!backwards.admit(":1.1").value);
    PrivilegedAdmission throwing(c,[]{throw 1;return Clock::time_point{};});assert(!throwing.admit(":1.1").value);
    c.global_burst=1;c.sender_per_second=1;const auto fixed=Clock::now();PrivilegedAdmission parallel(c,[=]{return fixed;});std::atomic<unsigned> admitted{};std::vector<std::thread> threads;for(unsigned i=0;i<32;++i)threads.emplace_back([&]{if(parallel.admit(":1.1").value==true)++admitted;});for(auto& t:threads)t.join();assert(admitted==1);
}
