#include "linux_startup_gsettings.hpp"
#include <cstring>
#ifdef __linux__
#include <dlfcn.h>
#endif
namespace pcinsight::detail {
namespace {
Metric<bool> unknown(const char* note) { return {std::nullopt,Support::unsupported,note}; }
bool identifier(const std::string& s) {
    if(s.empty() || s.size()>1024) return false;
    for(const auto c:s) if(!((c>='a'&&c<='z') || (c>='A'&&c<='Z') || (c>='0'&&c<='9') || c=='.' || c=='-' || c=='_')) return false;
    return true;
}
struct Release {
    void* pointer; void (*unref)(void*);
    ~Release() { if(pointer)unref(pointer); }
};
}
bool GSettingsApi::complete() const {
    return source && lookup && schema_unref && path && has_key && schema_key && key_unref && key_type && type_equal &&
        create && object_unref && value && value_type && boolean && value_unref;
}
Metric<bool> read_gsettings_boolean(const std::string& schema,const std::string& key,const GSettingsApi& api) {
    if(!identifier(schema) || !identifier(key)) return unknown("invalid/excessive GSettings identifier");
    if(!api.complete()) return unknown("GSettings runtime/symbols unavailable");
    auto* source=api.source();
    if(!source) return unknown("GSettings schema source unavailable"); // borrowed source
    Release description{api.lookup(source,schema.c_str(),1),api.schema_unref};
    if(!description.pointer) return unknown("GSettings schema not installed in caller context");
    if(!api.path(description.pointer)) return unknown("relocatable GSettings schema has no unambiguous path");
    if(!api.has_key(description.pointer,key.c_str())) return unknown("GSettings key not present in schema");
    Release definition{api.schema_key(description.pointer,key.c_str()),api.key_unref};
    if(!definition.pointer) return unknown("GSettings key definition unavailable");
    const auto* type=api.key_type(definition.pointer);
    if(!type || !api.type_equal(type,"b")) return unknown("GSettings condition key is not boolean");
    // Preflight prevents fatal GIO assertions for missing schemas/keys/type/path.
    Release settings{api.create(description.pointer,nullptr,nullptr),api.object_unref};
    if(!settings.pointer) return unknown("GSettings instance unavailable");
    Release value{api.value(settings.pointer,key.c_str()),api.value_unref};
    if(!value.pointer) return unknown("GSettings value unavailable");
    const auto* signature=api.value_type(value.pointer);
    if(!signature || std::strcmp(signature,"b")!=0) return unknown("GSettings returned non-boolean value");
    // Default/fallback values can be returned when the storage backend is absent.
    // GIO does not expose enough provenance to certify persisted user settings.
    return {api.boolean(value.pointer)!=0,Support::estimated,
        "GSettings effective caller value; may be schema default/backend fallback, not launch guarantee"};
}
Metric<bool> read_native_gsettings_boolean(const std::string& schema,const std::string& key) {
#ifdef __linux__
    static const auto api=[] {
        GSettingsApi a;
        // Retain for process lifetime: GIO registers types/modules/background state.
        // Unloading after a call could invalidate global callbacks in a host app.
        auto* library=dlopen("libgio-2.0.so.0",RTLD_NOW|RTLD_LOCAL);
        if(!library) return a;
#define LOAD(member,symbol) a.member=reinterpret_cast<decltype(a.member)>(dlsym(library,symbol))
        LOAD(source,"g_settings_schema_source_get_default");
        LOAD(lookup,"g_settings_schema_source_lookup");
        LOAD(schema_unref,"g_settings_schema_unref");
        LOAD(path,"g_settings_schema_get_path");
        LOAD(has_key,"g_settings_schema_has_key");
        LOAD(schema_key,"g_settings_schema_get_key");
        LOAD(key_unref,"g_settings_schema_key_unref");
        LOAD(key_type,"g_settings_schema_key_get_value_type");
        LOAD(type_equal,"g_variant_type_equal");
        LOAD(create,"g_settings_new_full");
        LOAD(object_unref,"g_object_unref");
        LOAD(value,"g_settings_get_value");
        LOAD(value_type,"g_variant_get_type_string");
        LOAD(boolean,"g_variant_get_boolean");
        LOAD(value_unref,"g_variant_unref");
#undef LOAD
        return a;
    }();
    return read_gsettings_boolean(schema,key,api);
#else
    (void)schema;(void)key; return unknown("native GSettings requires Linux");
#endif
}
}
