#include "smbios.hpp"
#include "cpu_identity_text.hpp"
#include <cerrno>
#include <fstream>
#include <limits>
#include <map>
#include <vector>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace pcinsight::detail {
namespace {
constexpr std::size_t max_table_bytes = 16*1024*1024;
using Bytes = std::span<const unsigned char>;
std::optional<std::string> cpu_string(Bytes strings,unsigned index){
    if(!index)return {};
    for(std::size_t start=0;start<strings.size();){
        auto end=start;while(end<strings.size()&&strings[end])++end;
        if(end==start||end==strings.size())return {};
        if(--index==0){
            auto text=cpu_identity_text(std::string_view(reinterpret_cast<const char*>(strings.data()+start),end-start));
            if(text&&(*text=="Unknown"||*text=="Not Specified"||*text=="To Be Filled By O.E.M."))return {};
            return text;
        }
        start=end+1;
    }
    return {};
}
std::uint16_t u16(Bytes b, std::size_t p) {
    return static_cast<std::uint16_t>(b[p] | (std::uint16_t(b[p+1]) << 8));
}
std::uint32_t u32(Bytes b, std::size_t p) {
    return std::uint32_t(u16(b,p)) | (std::uint32_t(u16(b,p+2)) << 16);
}
SmbiosInventory invalid(const char* note) {
    SmbiosInventory out;
    out.note=note;
    return out;
}
std::string memory_type(unsigned char v) {
    switch (v) {
        case 0x12: return "DDR"; case 0x13: return "DDR2"; case 0x18: return "DDR3";
        case 0x1a: return "DDR4"; case 0x1b: return "LPDDR"; case 0x1c: return "LPDDR2";
        case 0x1d: return "LPDDR3"; case 0x1e: return "LPDDR4"; case 0x22: return "DDR5";
        case 0x23: return "LPDDR5"; default: return {};
    }
}
std::string form_factor(unsigned char v) {
    switch (v) {
        case 0x09: return "DIMM"; case 0x0d: return "SODIMM"; case 0x0f: return "FB-DIMM";
        case 0x04: return "SIP"; case 0x05: return "Chip"; case 0x06: return "DIP";
        case 0x0a: return "TSOP"; case 0x0b: return "Row of chips"; default: return {};
    }
}
// Publish scalar summaries only when every installed device agrees and is known.
template<class T> struct Common {
    std::optional<T> value;
    bool valid=true;
    void add(std::optional<T> next) {
        if (!next || (value && *value!=*next)) valid=false;
        if (!value && next) value=next;
    }
    std::optional<T> get() const { return valid ? value : std::nullopt; }
};
}

SmbiosInventory parse_smbios(Bytes data) {
    if (data.size()<6 || data.size()>max_table_bytes) return invalid("SMBIOS table size invalid");
    struct Record {Bytes formatted,strings;};
    std::vector<Record> records;
    std::map<std::uint16_t, bool> arrays;
    for (std::size_t pos=0; pos<data.size();) {
        if (data.size()-pos<4) return invalid("Truncated SMBIOS header");
        const auto type=data[pos], length=data[pos+1];
        if (length<4 || length>data.size()-pos) return invalid("Invalid SMBIOS record length");
        auto next=pos+length;
        while (next+1<data.size() && (data[next]!=0 || data[next+1]!=0)) ++next;
        if (next+1>=data.size()) return invalid("Unterminated SMBIOS string set");
        const auto record=data.subspan(pos,length);
        if (type==16) {
            if (length<0x0f) return invalid("Truncated memory array");
            if (!arrays.emplace(u16(record,2),record[5]==3).second)
                return invalid("Duplicate memory array handle");
        }
        records.push_back({record,data.subspan(pos+length,next+2-(pos+length))});
        pos=next+2;
        if (type==127) break; // SMBIOS 3 tables may contain trailing padding.
    }
    SmbiosInventory out;
    out.support=Support::available;
    out.note="SMBIOS firmware inventory; scalar fields omitted when unknown or heterogeneous";
    std::uint64_t installed=0, used=0, total=0;
    bool sizes_known=true, occupancy_known=true;
    Common<std::uint64_t> speeds;
    Common<std::string> types, forms;
    Common<double> cpu_speeds;
    Common<std::string> cpu_vendors,cpu_models;
    for (auto record:records) {
        auto r=record.formatted;
        if(r[0]==4&&r.size()<0x19){
            cpu_speeds.add(std::nullopt);cpu_vendors.add(std::nullopt);cpu_models.add(std::nullopt);
        }else if (r[0]==4 && r[5]==3 && (r[0x18]&0x40)) {
            const auto mhz=u16(r,0x16);
            cpu_speeds.add(mhz && mhz!=0xffff ? std::optional<double>(mhz/1000.0) : std::nullopt);
            cpu_vendors.add(cpu_string(record.strings,r[7]));
            cpu_models.add(cpu_string(record.strings,r[0x10]));
        }
        if (r[0]!=17) continue;
        if (r.size()<0x15) return invalid("Truncated memory device");
        // Exclude video/flash arrays when firmware provides their ownership.
        if (!arrays.empty()) {
            const auto array=arrays.find(u16(r,4));
            if (array==arrays.end()) return invalid("Memory device references missing array");
            if (!array->second) continue;
        }
        ++total;
        const auto size=u16(r,0x0c);
        if (size==0) continue;
        if (size==0xffff) {
            sizes_known=false; occupancy_known=false;
        } else ++used;
        std::uint64_t bytes=0;
        if (size==0x7fff) {
            if (r.size()<0x20) sizes_known=false;
            else {
                const auto extended=u32(r,0x1c);
                if (!extended || (extended&0x80000000U)) sizes_known=false;
                else bytes=std::uint64_t(extended)*1024*1024;
            }
        } else if (size!=0xffff) {
            bytes=std::uint64_t(size&0x7fff)*((size&0x8000)?1024:1024*1024);
            if (!bytes) sizes_known=false;
        }
        if (bytes>std::numeric_limits<std::uint64_t>::max()-installed)
            return invalid("SMBIOS memory size overflow");
        installed+=bytes;
        const auto type=memory_type(r[0x12]), form=form_factor(r[0x0e]);
        types.add(type.empty()?std::nullopt:std::optional(type));
        forms.add(form.empty()?std::nullopt:std::optional(form));
        std::uint64_t speed=r.size()>=0x22 ? u16(r,0x20) : 0;
        if (speed==0xffff) {
            speed=r.size()>=0x5c ? u32(r,0x58) : 0;
            if (speed&0x80000000U) speed=0;
        }
        // Speed (0x15) is maximum capability, NOT the configured speed.
        speeds.add(speed ? std::optional(speed) : std::nullopt);
    }
    if (total) {
        out.slots_total=total;
        if (occupancy_known) out.slots_used=used;
        if (sizes_known) out.installed_memory_bytes=installed;
    }
    out.memory_type=types.get();
    out.memory_form_factor=forms.get();
    out.memory_speed_mts=speeds.get();
    out.firmware_cpu_ghz=cpu_speeds.get();
    out.cpu_vendor=cpu_vendors.get();out.cpu_model=cpu_models.get();
    if (total && arrays.empty()) {
        out.support=Support::estimated;
        out.note="SMBIOS type-17 records without array ownership; may include non-system memory";
    }
    return out;
}

SmbiosInventory parse_windows_smbios(Bytes buffer) {
    if (buffer.size()<8) return invalid("Truncated RawSMBIOSData header");
    const auto length=u32(buffer,4);
    if (length!=buffer.size()-8) return invalid("Invalid RawSMBIOSData length");
    return parse_smbios(buffer.subspan(8));
}

SmbiosInventory read_smbios(const std::string& table_path) {
    errno=0;
    std::ifstream file(table_path,std::ios::binary);
    if (!file) {
        auto out=invalid("Firmware/kernel does not expose an accessible SMBIOS table");
        if (errno==EACCES) { out.support=Support::permission_required; out.note="SMBIOS table requires privileges"; }
        return out;
    }
    std::vector<unsigned char> data;
    char buffer[4096];
    while (file.read(buffer,sizeof(buffer)) || file.gcount()) {
        const auto count=static_cast<std::size_t>(file.gcount());
        if (count>max_table_bytes-data.size()) return invalid("SMBIOS table exceeds size limit");
        data.insert(data.end(),buffer,buffer+count);
    }
    if (file.bad()) return invalid("SMBIOS table read failed");
    return parse_smbios(data);
}

#ifdef _WIN32
SmbiosInventory read_windows_smbios() {
    constexpr DWORD provider=0x52534d42; // 'RSMB', without multichar literals.
    UINT size=GetSystemFirmwareTable(provider,0,nullptr,0);
    for (int attempt=0; attempt<3; ++attempt) {
        if (!size) {
            auto out=invalid("GetSystemFirmwareTable failed");
            if (GetLastError()==ERROR_ACCESS_DENIED) out.support=Support::permission_required;
            return out;
        }
        if (size>max_table_bytes+8) return invalid("SMBIOS table exceeds size limit");
        std::vector<unsigned char> data(size);
        const auto written=GetSystemFirmwareTable(provider,0,data.data(),size);
        if (!written) {
            auto out=invalid("GetSystemFirmwareTable failed");
            if (GetLastError()==ERROR_ACCESS_DENIED) out.support=Support::permission_required;
            return out;
        }
        if (written>size) { size=written; continue; }
        data.resize(written);
        return parse_windows_smbios(data);
    }
    return invalid("SMBIOS table changed repeatedly during read");
}
#endif
} // namespace pcinsight::detail
