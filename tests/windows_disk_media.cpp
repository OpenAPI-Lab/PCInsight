#include "../src/windows_disk_media.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <iostream>
using namespace pcinsight;
using namespace pcinsight::detail;
namespace {
std::vector<DiskInfo> disks() {
    DiskInfo a,b; a.id="A"; b.id="B"; a.media_type=b.media_type="no-seek-penalty"; return {a,b};
}
const std::map<std::string,DiskHardwareIdentity> identities{{"A",{"serial A",500,11}},{"B",{"serial B",1000,17}}};
DiskMediaInventory inventory() {
    return {{{{"serial A",500,11},3},{{"serial B",1000,17},4}},Support::available,"fixture storage provider"};
}
}
int main() {
    VARIANT serial{},size{},bus{},medium{};
    serial.vt=VT_BSTR; serial.bstrVal=SysAllocString(L" SERIAL ");
    size.vt=VT_BSTR; size.bstrVal=SysAllocString(L"500");
    bus.vt=VT_I4; bus.lVal=11; medium.vt=VT_UI2; medium.uiVal=4;
    auto parsed=decode_windows_media_row(serial,size,bus,medium);
    assert(parsed&&parsed->identity.serial=="SERIAL"&&parsed->identity.capacity==500&&parsed->media_type==4);
    bus.lVal=-1; assert(!decode_windows_media_row(serial,size,bus,medium)); bus.lVal=11;
    VariantClear(&size); size.vt=VT_BSTR; size.bstrVal=SysAllocString(L"500bad");
    assert(!decode_windows_media_row(serial,size,bus,medium));
    VariantClear(&size); size.vt=VT_UI8; size.ullVal=500;
    assert(decode_windows_media_row(serial,size,bus,medium));
    VariantClear(&serial); serial.vt=VT_BSTR; serial.bstrVal=SysAllocStringLen(L"A\0B",3);
    assert(!decode_windows_media_row(serial,size,bus,medium));
    VariantClear(&serial); serial.vt=VT_NULL; assert(decode_windows_media_row(serial,size,bus,medium));
    medium.vt=VT_UI4; medium.ulVal=65536; assert(!decode_windows_media_row(serial,size,bus,medium));
    VariantClear(&serial); VariantClear(&size); VariantClear(&bus); VariantClear(&medium);
    auto d=disks(); auto rows=inventory(); apply_windows_disk_media(d,identities,rows);
    assert(d[0].reported_media_type.value=="HDD" && d[1].reported_media_type.value=="SSD");
    assert(d[0].media_type=="HDD"&&d[1].media_type=="SSD");
    rows.rows[0].media_type=5; apply_windows_disk_media(d,identities,rows);
    assert(d[0].reported_media_type.value=="SCM");
    rows.rows[0].media_type=0; apply_windows_disk_media(d,identities,rows);
    assert(!d[0].reported_media_type.value&&d[0].media_type.empty());
    rows.rows[0].media_type=99; apply_windows_disk_media(d,identities,rows); assert(!d[0].reported_media_type.value);
    for(int mismatch=0;mismatch<3;++mismatch) {
        d=disks(); rows=inventory();
        if(mismatch==0)rows.rows[0].identity.serial="other serial";
        if(mismatch==1)rows.rows[0].identity.capacity++;
        if(mismatch==2)rows.rows[0].identity.bus=10;
        apply_windows_disk_media(d,identities,rows);
        assert(!d[0].reported_media_type.value && d[0].media_type=="no-seek-penalty");
        assert(d[1].reported_media_type.value=="SSD");
    }
    d=disks(); rows=inventory(); rows.rows.push_back(rows.rows[0]);
    apply_windows_disk_media(d,identities,rows); assert(!d[0].reported_media_type.value);
    auto duplicate_native=identities; duplicate_native["alias A"]=identities.at("A");
    rows=inventory(); apply_windows_disk_media(d,duplicate_native,rows); assert(!d[0].reported_media_type.value);
    auto missing=identities; missing["A"].serial.clear();
    apply_windows_disk_media(d,missing,rows); assert(!d[0].reported_media_type.value);
    missing["A"]=identities.at("A"); missing["A"].bus=0;
    apply_windows_disk_media(d,missing,rows); assert(!d[0].reported_media_type.value);
    DiskMediaInventory denied{{},Support::permission_required,"denied"};
    apply_windows_disk_media(d,identities,denied);
    assert(!d[1].reported_media_type.value && d[1].reported_media_type.support==Support::permission_required);
    unsigned calls=0; auto current=inventory();
    WindowsDiskMedia cache([&] { ++calls; return current; });
    const auto now=std::chrono::steady_clock::now();
    d=disks(); cache.fill(d,identities,now); assert(calls==1 && d[0].reported_media_type.value=="HDD");
    cache.fill(d,identities,now+std::chrono::seconds(29)); assert(calls==1);
    current=denied; cache.fill(d,identities,now+std::chrono::seconds(30));
    assert(calls==2 && !d[0].reported_media_type.value && d[0].media_type.empty());
    auto replacement=identities; replacement["A"].serial="replacement";
    current=inventory(); cache.fill(d,replacement,now+std::chrono::seconds(31)); assert(calls==3 && !d[0].reported_media_type.value);
    cache.fill(d,replacement,now); assert(calls==4); // Monotonic regression also invalidates cached metadata.
    cache.reset(); cache.fill(d,identities,now); assert(calls==5);
    cache.fill(d,{},now); assert(!d[0].reported_media_type.value);
    Monitor monitor; SampleOptions options; options.include_processes=false; options.include_networks=false; options.include_gpus=false;
    auto native=monitor.sample(options); unsigned classified=0;
    for(const auto& disk:native.disks) if(disk.reported_media_type.value) {
        ++classified; assert(disk.reported_media_type.support==Support::available);
        assert(*disk.reported_media_type.value=="HDD"||*disk.reported_media_type.value=="SSD"||*disk.reported_media_type.value=="SCM");
    }
    std::cout<<"Native classified disks: "<<classified<<'/'<<native.disks.size()<<'\n';
    for(const auto& disk:native.disks) std::cout<<disk.model<<": "<<disk.reported_media_type.value.value_or("unavailable")
        <<"; "<<disk.reported_media_type.note<<'\n';
    options.include_disk_media=false;
    for(const auto& disk:monitor.sample(options).disks) assert(!disk.reported_media_type.value);
    options.include_disks=false; assert(monitor.sample(options).disks.empty());
    // The library must preserve a caller-owned STA apartment.
    const auto initialized=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    if(SUCCEEDED(initialized)) {
        const auto queried=read_windows_disk_media(); (void)queried;
        const auto existing=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
        assert(existing==S_FALSE); CoUninitialize(); CoUninitialize();
    }
}
