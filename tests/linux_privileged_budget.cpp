#include "../src/linux_privileged_budget.hpp"
#include <cassert>
using namespace pcinsight::detail;
namespace {struct Bus{std::uint64_t timeout=2000000;bool fail_set{};};int get(Bus* b,std::uint64_t* t){*t=b->timeout;return 0;}int set(Bus* b,std::uint64_t t){if(b->fail_set)return -1;b->timeout=t;return 0;}}
int main(){using Clock=PrivilegedRequestBudget::Clock;auto now=Clock::time_point{};auto budget=std::make_shared<PrivilegedRequestBudget>(1000000,3,[&]{return now;});
    assert(budget->live()&&budget->used()==0);assert(budget->claim(5000000).value==1000000);now+=std::chrono::microseconds(250000);assert(budget->claim(200000).value==200000);assert(budget->claim().value==750000);assert(!budget->claim().value&&budget->used()==3);assert(budget->live());now+=std::chrono::microseconds(750000);assert(!budget->live()&&!budget->claim().value);
    for(auto timeout:{0ULL,999ULL,30000001ULL}){PrivilegedRequestBudget b(timeout,1);assert(!b.live()&&!b.claim().value);}for(auto limit:{0U,129U}){PrivilegedRequestBudget b(1000,limit);assert(!b.live()&&!b.claim().value);}
    PrivilegedRequestBudget maximum(30000000,128);assert(maximum.live());for(unsigned i=0;i<128;++i)assert(maximum.claim().value);assert(!maximum.claim().value&&maximum.used()==128);
    PrivilegedRequestBudget rollback(1000,2,[&]{return now;});now-=std::chrono::microseconds(1);assert(!rollback.live()&&!rollback.claim().value);
    PrivilegedRequestBudget throwing(1000,1,[]{throw 1;return Clock::time_point{};});assert(!throwing.live()&&!throwing.claim().value);
    auto scoped=std::make_shared<PrivilegedRequestBudget>(100000,10);Bus bus;
    {PrivilegedRpcTimeout lease(&bus,get,set,scoped);assert(bus.timeout>0&&bus.timeout<=100000);}assert(bus.timeout==2000000);
    try{PrivilegedRpcTimeout lease(&bus,get,set,scoped);throw 1;}catch(...){}assert(bus.timeout==2000000);
    bus.fail_set=true;bool rejected{};try{PrivilegedRpcTimeout lease(&bus,get,set,scoped);}catch(...){rejected=true;}assert(rejected&&bus.timeout==2000000);
    assert(privileged_rpc_timeout({})==5000000);
}
