#include "../src/windows_pdh.hpp"
#include <pdhmsg.h>
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <thread>

using namespace pcinsight;
using namespace pcinsight::detail;
namespace {
struct Fake {
    PDH_STATUS open_status=0, add_status=0, collect_status=0, format_status=0;
    DWORD data_status=PDH_CSTATUS_VALID_DATA;
    double value=37.5;
    int opens=0, closes=0, formats=0;
} fake;
int query_token, counter_token;
PDH_STATUS WINAPI open_query(LPCWSTR source,DWORD_PTR,PDH_HQUERY* out) {
    assert(!source); ++fake.opens;
    if (!fake.open_status) *out=&query_token;
    return fake.open_status;
}
PDH_STATUS WINAPI add_counter(PDH_HQUERY query,LPCWSTR path,DWORD_PTR,PDH_HCOUNTER* out) {
    assert(query==&query_token);
    assert(std::wstring(path)==L"\\Processor Information(_Total)\\% Processor Time");
    if (!fake.add_status) *out=&counter_token;
    return fake.add_status;
}
PDH_STATUS WINAPI collect(PDH_HQUERY query) {
    assert(query==&query_token); return fake.collect_status;
}
PDH_STATUS WINAPI format(PDH_HCOUNTER counter,DWORD flags,LPDWORD,PPDH_FMT_COUNTERVALUE out) {
    assert(counter==&counter_token);
    assert(flags==(PDH_FMT_DOUBLE|PDH_FMT_NOCAP100));
    ++fake.formats; out->CStatus=fake.data_status; out->doubleValue=fake.value;
    return fake.format_status;
}
PDH_STATUS WINAPI close_query(PDH_HQUERY query) {
    assert(query==&query_token); ++fake.closes; return ERROR_SUCCESS;
}
PdhApi api() { return {open_query,add_counter,collect,format,close_query}; }
void warm(WindowsCpuCounter& counter,unsigned n=128,unsigned g=2) {
    auto r=counter.sample(n,g);
    assert(!r.value && r.support==Support::unsupported);
}
}
int main() {
    {
        WindowsCpuCounter c(api());
        warm(c); assert(fake.formats==0);
        assert(c.sample(128,2).value==37.5);
        fake.data_status=PDH_CSTATUS_NEW_DATA; fake.value=0;
        assert(c.sample(128,2).value==0); // genuine zero remains available
        fake.value=100; assert(c.sample(128,2).value==100);
        warm(c,192,3); assert(fake.closes==1); // changed topology invalidates baseline
        assert(c.sample(192,3).value==100);
        c.reset(); assert(fake.closes==2);
        warm(c,192,3);
    }
    assert(fake.opens==fake.closes);
    for (int stage=0;stage<4;++stage) {
        fake={};
        {
            WindowsCpuCounter c(api());
            if (stage==0) fake.open_status=PDH_ACCESS_DENIED;
            if (stage==1) fake.add_status=PDH_ACCESS_DENIED;
            if (stage==2) fake.collect_status=PDH_ACCESS_DENIED;
            if (stage==3) { warm(c); fake.format_status=PDH_ACCESS_DENIED; }
            auto r=c.sample(128,2);
            assert(!r.value && r.support==Support::permission_required);
            fake.open_status=fake.add_status=fake.collect_status=fake.format_status=0;
            warm(c); // recovery cannot publish a stale delta
            assert(c.sample(128,2).value==37.5);
        }
        assert(fake.closes==fake.opens-(stage==0?1:0));
    }
    fake={};
    {
        WindowsCpuCounter c(api()); warm(c);
        fake.data_status=PDH_CSTATUS_NO_INSTANCE;
        assert(!c.sample(128,2).value);
        fake.data_status=PDH_CSTATUS_VALID_DATA;
        warm(c); assert(c.sample(128,2).value==37.5);
        for (double v : {-1.0,101.0,std::numeric_limits<double>::infinity(),
                         std::numeric_limits<double>::quiet_NaN()}) {
            fake.value=v; assert(!c.sample(128,2).value); warm(c);
        }
        assert(!c.sample(0,0).value);
        warm(c); fake.value=10; assert(c.sample(128,2).value==10);
    }
    assert(fake.opens==fake.closes);
    // Actual PDH provider, including on the single-group development machine.
    WindowsCpuCounter live;
    const auto count=GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
    const auto groups=GetActiveProcessorGroupCount();
    assert(!live.sample(count,groups).value);
    Metric<double> result;
    for (int i=0;i<5 && !result.value;++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        result=live.sample(count,groups);
    }
    if (!result.value) std::cerr << result.note << '\n';
    assert(result.value && std::isfinite(*result.value) && *result.value>=0 && *result.value<=100);
    std::cout << "Native PDH CPU total: " << *result.value << "% (" << groups << " groups)\n";
}
