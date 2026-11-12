#include "pcinsight/startup_recorder.hpp"
#include <algorithm>
#include <charconv>
#include <iostream>
#include <string_view>
#include <thread>
using namespace pcinsight;
int main(int argc,char** argv){
    if(argc==2&&std::string_view(argv[1])=="--help"){
        std::cout<<"pcinsight-startup-recorder --accept-estimated-gio FILE CAPTURE_ID [DURATION_MS]\n"
            "Optional Linux capture host, default60000ms/range10..3600000ms.\n"
            "Explicit invocation only; no installation, autostart activation, privilege elevation or app launch.\n"
            "GIO hints are untrusted/manual-capable; automatic startup NOT certified.\n"
            "Stores historical observations, not global discovery coverage or resumable state.\n";return 0;
    }
    if(argc<4||argc>5||std::string_view(argv[1])!="--accept-estimated-gio"){std::cerr<<"Explicit --accept-estimated-gio FILE CAPTURE_ID [DURATION_MS] required; use --help.\n";return 2;}
    StartupRecorderOptions options;options.capture_id=argv[3];options.accept_estimated_gio_associations=true;
    if(argc==5){std::int64_t n{};const std::string_view text=argv[4];const auto parsed=std::from_chars(text.data(),text.data()+text.size(),n);
        if(parsed.ec!=std::errc{}||parsed.ptr!=text.data()+text.size()||n<10||n>3600000){std::cerr<<"Invalid duration.\n";return 2;}options.duration=std::chrono::milliseconds(n);}
    if(options.capture_id.empty()||options.capture_id.size()>256){std::cerr<<"Invalid capture ID.\n";return 2;}
    try{
        const std::filesystem::path path=argv[2];std::error_code error;
        if(path.empty()||path.filename().empty()||path.filename()=="."||path.filename()==".."){std::cerr<<"Invalid archive path.\n";return 2;}
        const auto status=std::filesystem::symlink_status(path,error);
        if((error&&error!=std::errc::no_such_file_or_directory)||(std::filesystem::exists(status)&&!StartupImpactStore::load(path))){std::cerr<<"Existing archive unreadable/invalid; preserved.\n";return 4;}
        const auto parent=path.parent_path().empty()?std::filesystem::path("."):path.parent_path();
        if(!std::filesystem::is_directory(parent,error)||error){std::cerr<<"Archive directory must already exist.\n";return 4;}
        LinuxStartupRecorder recorder;const auto start=recorder.start(options);
        if(!start){std::cerr<<"Native recorder unavailable; no capture saved.\n";return start.error==ActionError::invalid_argument?2:3;}
        std::cout<<"Recorder ready: "<<(recorder.report().state==StartupRecorderState::recording?"observation phase active":"phase already closed during initial scan/setup")<<"; estimated hints only.\n"<<std::flush;
        for(;;){const auto report=recorder.report();if(report.state==StartupRecorderState::closed)break;
            if(report.state!=StartupRecorderState::recording){std::cerr<<"Recorder failed; no successful archive.\n";return 5;}
            // No long blocking wait/catch-up storms. Host owns scheduling.
            std::this_thread::sleep_until(std::min(report.next_scan,std::chrono::steady_clock::now()+std::chrono::milliseconds(250)));
            if(!recorder.poll()){std::cerr<<"Recorder poll failed; no successful archive.\n";return 5;}}
        const auto saved=recorder.save(path);if(!saved){std::cerr<<(saved.replaced?"Archive published; durability confirmation failed.\n":"Archive save failed.\n");return 4;}
        const auto report=recorder.report();
        std::cout<<"Historical observations="<<report.observations.size()<<" scans="<<report.scans<<" partial_scans="<<report.partial_scans<<" unavailable="<<report.unavailable<<" rejected="<<report.rejected<<".\n"
            "Historical recorder diagnostics are stored in archive v2. No certified autostart origin, retroactive coverage or short-lived foreign-process guarantee.\n";
        return 0;
    }catch(...){std::cerr<<"Recorder failed; no successful completion claimed.\n";return 5;}
}
