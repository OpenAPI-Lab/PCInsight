#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
namespace pcinsight::detail {
// Bounded UTF-8 scalar text, no ASCII/C1 controls; normalize spaces/tabs only.
inline std::optional<std::string> cpu_identity_text(std::string_view s){
    if(s.size()>1024)return {};
    std::string out;bool space=false;
    for(std::size_t i=0;i<s.size();){
        const auto c=static_cast<unsigned char>(s[i]);
        if(c==' '||c=='\t'){space=!out.empty();++i;continue;}
        if(c<32||c==127)return {};
        std::size_t width=1;std::uint32_t point=c;
        if(c>=128){
            if(c>=0xc2&&c<=0xdf){width=2;point=c&31;}
            else if(c>=0xe0&&c<=0xef){width=3;point=c&15;}
            else if(c>=0xf0&&c<=0xf4){width=4;point=c&7;}
            else return {};
            if(width>s.size()-i)return {};
            for(std::size_t n=1;n<width;++n){const auto tail=static_cast<unsigned char>(s[i+n]);if((tail&0xc0)!=0x80)return {};point=(point<<6)|(tail&63);}
            if((width==3&&point<0x800)||(width==4&&point<0x10000)||point>0x10ffff||(point>=0xd800&&point<=0xdfff)||(point>=0x80&&point<=0x9f))return {};
        }
        if(space)out+=' ';
        space=false;out.append(s.substr(i,width));i+=width;
    }
    if(out.empty())return {};
    return out;
}
}
