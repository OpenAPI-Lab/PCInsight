#pragma once
#include "pcinsight/types.hpp"
namespace pcinsight::detail {
// ABI-only loader: no build-time GLib/GIO headers or dependency.
struct GSettingsApi {
    void* (*source)(){};
    void* (*lookup)(void*,const char*,int){};
    void (*schema_unref)(void*){};
    const char* (*path)(void*){};
    int (*has_key)(void*,const char*){};
    void* (*schema_key)(void*,const char*){};
    void (*key_unref)(void*){};
    const void* (*key_type)(void*){};
    int (*type_equal)(const void*,const void*){};
    void* (*create)(void*,void*,const char*){};
    void (*object_unref)(void*){};
    void* (*value)(void*,const char*){};
    const char* (*value_type)(void*){};
    int (*boolean)(void*){};
    void (*value_unref)(void*){};
    bool complete() const;
};
Metric<bool> read_gsettings_boolean(const std::string&,const std::string&,const GSettingsApi&);
Metric<bool> read_native_gsettings_boolean(const std::string&,const std::string&);
}
