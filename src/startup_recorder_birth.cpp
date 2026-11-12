#include "startup_recorder.hpp"
namespace pcinsight::detail {
StartupRecorderBirthResult make_native_startup_recorder_birth_source(bool authorized,int fd,std::uint32_t pid,std::uint64_t ticks,const StartupBirthObserverOptions& options){
    if(!authorized)return {{},{ActionError::permission_denied,"Explicit selected-creator birth capture authorization required"}};
    try{
        auto observer=std::make_shared<LinuxStartupBirthObserver>();
        const auto result=observer->start(true,fd,pid,ticks,options);if(!result)return {{},result};
        StartupRecorderBirthSource source{
            [observer]{return observer->poll();},[observer]{return observer->report();},
            [observer](const auto& key,const auto& id){return observer->associate(key,id);},[observer]{observer->close();}};
        return {std::move(source),{}};
    }catch(...){return {{},{ActionError::system_error,"Native birth source construction failed; owned references released"}};}
}
StartupRecorderManualSource make_native_startup_recorder_manual_source(){
    auto session=std::make_shared<LinuxDesktopResourceSession>();
    return {[session](const auto& id,bool launch,bool capture,const auto& name){return session->start(id,launch,capture,name);},
        [session](auto window,const auto& policy){return session->finish(window,policy);},[session]{session->stop();}};
}
}
