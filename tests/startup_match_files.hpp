#pragma once
#ifdef __linux__
#include <filesystem>
#include <fstream>
#include <map>
#include <cstdlib>
#include <stdexcept>
#include <unistd.h>
struct StartupMatchFiles {
    static void require(bool v){if(!v)throw std::runtime_error("private startup catalog fixture failed");}
    std::filesystem::path path,personal;
    std::map<std::string,std::optional<std::string>> saved;
    StartupMatchFiles(){char p[]="/tmp/pcinsight-exec-match-XXXXXX";const auto r=::mkdtemp(p);require(r!=nullptr);path=r;personal=path/"config/autostart";
        std::filesystem::create_directories(personal);std::filesystem::create_directories(path/"system/autostart");
        env("XDG_CONFIG_HOME",(path/"config").string());env("XDG_CONFIG_DIRS",(path/"system").string());env("XDG_CURRENT_DESKTOP","PCINSIGHT");}
    void env(const char* k,const std::string& v){const auto old=::getenv(k);saved[k]=old?std::optional<std::string>(old):std::nullopt;require(::setenv(k,v.c_str(),1)==0);}
    std::string desktop(const char* filename,const std::string& command,bool disabled=false){const auto p=personal/filename;std::ofstream out(p);require(static_cast<bool>(out));out<<"[Desktop Entry]\nType=Application\nName=Private fixture\nExec="<<command<<"\n"<<(disabled?"Hidden=true\n":"");out.close();require(static_cast<bool>(out));return p.string();}
    ~StartupMatchFiles(){for(const auto& [k,v]:saved){if(v)::setenv(k.c_str(),v->c_str(),1);else ::unsetenv(k.c_str());}std::error_code error;std::filesystem::remove_all(path,error);}
};
#endif
