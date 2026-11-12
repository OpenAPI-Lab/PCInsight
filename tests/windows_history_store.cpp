#include "pcinsight/windows_history_store.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>
using namespace pcinsight;
namespace fs=std::filesystem;
namespace {
struct Temp {
    fs::path path;
    Temp(){const auto stamp=std::chrono::steady_clock::now().time_since_epoch().count();for(unsigned i=0;i<100;++i){path=fs::temp_directory_path()/("pcinsight-whistory-"+std::to_string(stamp)+"-"+std::to_string(i));if(fs::create_directory(path))return;}throw std::runtime_error("no private test directory");}
    ~Temp(){for(const auto* name:{"history.bin","test.bin","legacy.bin","history.bin.backup","destination-dir"}){std::error_code ec;fs::remove(path/name,ec);}std::error_code ec;fs::remove(path,ec);} // Exact fixture names only, no recursion.
};
void write(const fs::path& path,const std::string& bytes){std::ofstream out(path,std::ios::binary);assert(out);out.write(bytes.data(),static_cast<std::streamsize>(bytes.size()));out.close();assert(out);}
std::string read(const fs::path& path){std::ifstream in(path,std::ios::binary);return {std::istreambuf_iterator<char>(in),{}};}
void set(std::string& text,std::size_t offset,std::uint64_t value,unsigned count){for(unsigned i=0;i<count;++i)text.at(offset+i)=static_cast<char>((value>>(i*8))&255);}
std::string signed_bytes(std::string bytes){bytes.resize(bytes.size()-8);std::uint64_t hash=14695981039346656037ULL;for(unsigned char c:bytes){hash^=c;hash*=1099511628211ULL;}for(unsigned i=0;i<8;++i)bytes+=static_cast<char>((hash>>(i*8))&255);return bytes;}
WindowsAppHistoryEntry entry(){
    WindowsAppHistoryEntry row;row.id="C:\\App.exe";row.name="App \xc3\xa9";row.period_start=std::chrono::system_clock::time_point(std::chrono::milliseconds(1000));row.period_end=row.period_start+std::chrono::milliseconds(2000);
    row.observed_cpu_time={std::chrono::nanoseconds(123456789),Support::estimated,"observed CPU, not complete coverage"};
    row.network_bytes={{},Support::unsupported,"no network provider"};row.notifications={{},Support::permission_required,"fixture permission denial"};return row;
}
template<class F> void invalid(F f){bool threw=false;try{f();}catch(const std::invalid_argument&){threw=true;}assert(threw);}
}
int main(){
    Temp temp;const auto path=temp.path/"history.bin",test=temp.path/"test.bin";
    auto row=entry();std::vector<WindowsAppHistoryEntry> rows{row};
    assert(WindowsHistoryStore::save(path,rows));const auto original=read(path);auto loaded=WindowsHistoryStore::load(path);
    assert(loaded && loaded.format_version==1 && loaded.entries.size()==1);
    const auto& restored=loaded.entries.front();
    assert(restored.id==row.id && restored.name==row.name && restored.observed_cpu_time.value==row.observed_cpu_time.value && restored.observed_cpu_time.support==Support::estimated && restored.observed_cpu_time.note==row.observed_cpu_time.note);
    assert(!restored.network_bytes.value && restored.network_bytes.support==Support::unsupported && restored.network_bytes.note==row.network_bytes.note);
    assert(!restored.notifications.value && restored.notifications.support==Support::permission_required && restored.notifications.note==row.notifications.note);
    auto zero=row;zero.observed_cpu_time={std::chrono::nanoseconds(0),Support::available,"known zero"};zero.network_bytes={0,Support::estimated,"known network zero"};zero.notifications={0,Support::available,"known notification zero"};
    assert(WindowsHistoryStore::save(test,std::vector{zero}));loaded=WindowsHistoryStore::load(test);assert(loaded && loaded.entries[0].network_bytes.value==0 && loaded.entries[0].notifications.value==0 && loaded.entries[0].observed_cpu_time.value->count()==0);
    auto missing=row;missing.observed_cpu_time={{},Support::permission_required,"denied CPU"};assert(WindowsHistoryStore::save(test,std::vector{missing}));loaded=WindowsHistoryStore::load(test);assert(loaded && !loaded.entries[0].observed_cpu_time.value && loaded.entries[0].observed_cpu_time.support==Support::permission_required);
    auto merged=WindowsHistoryStore::merge(rows,rows);assert(merged.size()==1 && merged[0].observed_cpu_time.value==row.observed_cpu_time.value);
    auto newer=row;newer.period_end+=std::chrono::milliseconds(100);newer.observed_cpu_time.value=std::chrono::nanoseconds(200000000);
    merged=WindowsHistoryStore::merge(rows,std::vector{newer});assert(merged.size()==1 && merged[0].observed_cpu_time.value==newer.observed_cpu_time.value);
    merged=WindowsHistoryStore::merge(std::vector{newer},rows);assert(merged[0].observed_cpu_time.value==newer.observed_cpu_time.value);
    auto decreased=newer;decreased.observed_cpu_time.value=std::chrono::nanoseconds(1);invalid([&]{WindowsHistoryStore::merge(rows,std::vector{decreased});});
    missing.period_end=newer.period_end;invalid([&]{WindowsHistoryStore::merge(rows,std::vector{missing});});
    auto separate=newer;separate.period_start+=std::chrono::seconds(1);assert(WindowsHistoryStore::merge(rows,std::vector{separate}).size()==2);
    assert(WindowsHistoryStore::retain_since(rows,row.period_end).size()==1);assert(WindowsHistoryStore::retain_since(rows,row.period_end+std::chrono::milliseconds(1)).empty());
    for(std::size_t length=0;length<original.size();++length){write(test,original.substr(0,length));loaded=WindowsHistoryStore::load(test);assert(!loaded && loaded.entries.empty());}
    auto corrupt=original;corrupt[20]^=1;write(test,corrupt);assert(!WindowsHistoryStore::load(test));const auto bad=read(test);assert(!WindowsHistoryStore::prune(test,row.period_end));assert(!WindowsHistoryStore::save(test,rows));assert(read(test)==bad);
    auto reject=[&](std::string data){write(test,signed_bytes(std::move(data)));const auto result=WindowsHistoryStore::load(test);assert(!result && result.entries.empty());};
    corrupt=original;set(corrupt,8,99,4);reject(corrupt);
    corrupt=original;set(corrupt,12,100001,4);reject(corrupt);
    corrupt=original;set(corrupt,16,1048577,4);reject(corrupt);
    const auto time=16+4+row.id.size()+4+row.name.size(),metric=time+16;
    corrupt=original;set(corrupt,time,5000,8);reject(corrupt); // Inverted interval.
    corrupt=original;set(corrupt,metric,99,1);reject(corrupt);
    corrupt=original;set(corrupt,metric+1,0,1);reject(corrupt); // Known support with absent flag.
    corrupt=original;set(corrupt,metric+1,2,1);reject(corrupt);
    corrupt=original;set(corrupt,metric+2,std::numeric_limits<std::uint64_t>::max(),8);reject(corrupt);
    corrupt=original;corrupt.insert(corrupt.end()-8,'x');reject(corrupt);
    const auto legacy=temp.path/"legacy.bin";assert(HistoryStore::save(legacy,{}));const auto old=read(legacy);assert(!WindowsHistoryStore::load(legacy));assert(!WindowsHistoryStore::save(legacy,rows));assert(read(legacy)==old);assert(!HistoryStore::load(path));
    write(temp.path/"history.bin.backup","unrelated backup");auto invalid_row=row;invalid_row.network_bytes={0,Support::unsupported,"invalid presence"};assert(!WindowsHistoryStore::save(path,std::vector{invalid_row}));assert(read(path)==original);
    invalid_row=row;invalid_row.observed_cpu_time.value=std::chrono::nanoseconds(-1);assert(!WindowsHistoryStore::save(path,std::vector{invalid_row}));
    invalid_row=row;invalid_row.notifications.support=static_cast<Support>(99);assert(!WindowsHistoryStore::save(path,std::vector{invalid_row}));
    // One negative native clock tick is representable even when system_clock
    // uses 100ns ticks (MSVC); duration_cast(-1ns) would truncate to zero there.
    invalid_row=row;invalid_row.period_start=std::chrono::system_clock::time_point(std::chrono::system_clock::duration(-1));assert(!WindowsHistoryStore::save(path,std::vector{invalid_row}));
    fs::create_directory(temp.path/"destination-dir");assert(!WindowsHistoryStore::save(temp.path/"destination-dir",rows));assert(fs::is_directory(temp.path/"destination-dir"));
    assert(read(temp.path/"history.bin.backup")=="unrelated backup");for(const auto& item:fs::directory_iterator(temp.path))assert(item.path().filename().string().find(".tmp-")==std::string::npos);
    assert(WindowsHistoryStore::prune(path,row.period_end+std::chrono::milliseconds(1)));loaded=WindowsHistoryStore::load(path);assert(loaded && loaded.entries.empty());
    assert(WindowsHistoryStore::reset(path));assert(WindowsHistoryStore::reset(path));assert(!WindowsHistoryStore::load(path));assert(!WindowsHistoryStore::reset({}));
    assert(!WindowsHistoryStore::save({},rows));assert(!WindowsHistoryStore::load({}));
    std::cout<<"Windows history store: presence/support/notes, measured zero, cumulative merge, bounds, all truncations, corruption, separate formats and atomic replacement failures passed\n";
}
