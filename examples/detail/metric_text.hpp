#pragma once
#include <pcinsight/metric.hpp>
#include <cmath>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <type_traits>
namespace pcinsight_example {
inline const char* support_name(pcinsight::Support s){
    using pcinsight::Support;
    switch(s){case Support::available:return "available";case Support::estimated:return "estimated";
        case Support::permission_required:return "permission_required";case Support::unsupported:return "unsupported";}
    return "invalid_support";
}
inline std::string escaped(const std::string& text){
    std::ostringstream out;
    for(unsigned char c:text){if(c<32||c==127||c=='\\')out<<"\\x"<<std::hex<<std::setw(2)<<std::setfill('0')<<unsigned(c);else out<<char(c);}
    return out.str(); // Terminal demonstration, NOT a JSON/UTF-8 serializer.
}
template<class T>void field(std::ostream& out,const std::string& key,const char* unit,const pcinsight::Metric<T>& m){
    bool known=pcinsight::metric_is_known(m);
    if constexpr(std::is_floating_point_v<T>)if(m.value&&!std::isfinite(*m.value))known=false;
    out<<escaped(key)<<'\t'<<unit<<'\t'<<support_name(m.support)<<'\t';
    if(!known)out<<"unknown";
    else if constexpr(std::is_same_v<T,std::string>)out<<std::quoted(escaped(*m.value));
    else if constexpr(requires{m.value->count();})out<<m.value->count();
    else out<<std::boolalpha<<*m.value;
    out<<'\t'<<std::quoted(escaped(m.note))<<'\n';
}
}
