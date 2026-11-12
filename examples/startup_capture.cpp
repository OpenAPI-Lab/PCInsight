#include "pcinsight/monitor.hpp"
#include "pcinsight/startup_candidates.hpp"
#include "pcinsight/startup_observer.hpp"
#include "pcinsight/startup_store.hpp"
#include <algorithm>
#include <charconv>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>
#include <string_view>
#include <thread>
#ifdef __linux__
#include <unistd.h>
#endif
using namespace pcinsight;
using namespace std::chrono_literals;
int main(int argc,char** argv) {
    if(argc==2 && std::string_view(argv[1])=="--help") {
        std::cout<<"pcinsight-startup-capture FILE CAPTURE_ID [DURATION_MS]\n"
            "Linux reference collector; default 60000ms, range 10..3600000ms.\n"
            "Explicit invocation accepts estimated launch-marker associations, NOT proof of autostart.\n"
            "Replaces a valid historical capture at FILE; refuses corrupt existing captures.\n"
            "No app launch, startup installation, hidden daemon, or external utilities.\n";
        return 0;
    }
    std::int64_t milliseconds=60000;
    if(argc<3 || argc>4){std::cerr<<"Expected FILE CAPTURE_ID [DURATION_MS]; use --help.\n";return 2;}
    const std::string capture_id=argv[2];
    if(capture_id.empty() || capture_id.size()>256){std::cerr<<"Invalid capture ID.\n";return 2;}
    if(argc==4) {
        const std::string_view text=argv[3];const auto parsed=std::from_chars(text.data(),text.data()+text.size(),milliseconds);
        if(parsed.ec!=std::errc{} || parsed.ptr!=text.data()+text.size() || milliseconds<10 || milliseconds>3600000) {
            std::cerr<<"Duration must be an integer from 10 to 3600000ms.\n";return 2;
        }
    }
#ifndef __linux__
    std::cerr<<"This reference collector requires the native Linux candidate provider.\n";
    return 3;
#else
    try {
        const std::filesystem::path path=argv[1];
        if(path.empty() || path.filename().empty() || path.filename()=="." || path.filename()=="..") {
            std::cerr<<"Invalid archive file path.\n";return 2;
        }
        std::error_code error;
        const auto status=std::filesystem::symlink_status(path,error);
        if(error && error!=std::errc::no_such_file_or_directory) {std::cerr<<"Cannot inspect archive path.\n";return 4;}
        if(std::filesystem::exists(status) && !StartupImpactStore::load(path)) {
            std::cerr<<"Existing capture cannot be loaded; refusing automatic overwrite.\n";return 4;
        }
        const auto parent=path.parent_path().empty()?std::filesystem::path("."):path.parent_path();
        if(!std::filesystem::is_directory(parent,error) || error){std::cerr<<"Archive directory must already exist.\n";return 4;}
        Monitor monitor;SampleOptions options;
        options.include_disks=options.include_networks=options.include_gpus=options.include_disk_media=false;
        options.include_startup_entries=true;
        auto snapshot=monitor.sample(options);std::uint64_t own_ticks=0;
        for(const auto& process:snapshot.processes)if(process.pid==static_cast<std::uint32_t>(getpid()))
            own_ticks=process.linux_start_ticks.value.value_or(0);
        const auto own=read_startup_process_sample(getpid(),own_ticks);
        if(!own){std::cerr<<"Native sampling context unavailable; no capture saved.\n";return 3;}
        const auto context=own.value->native_context;
        const auto window=std::chrono::milliseconds(milliseconds);
        StartupImpactObserver observer(window);
        const auto native_window=std::chrono::duration_cast<std::chrono::steady_clock::duration>(window);
        std::set<std::pair<std::uint32_t,std::uint64_t>> registered;
        std::size_t partial_scans=0,unavailable=0,rejected=0;
        auto scan=[&](SystemSnapshot current) {
            std::erase_if(current.processes,[&](const ProcessInfo& process){
                return registered.contains({process.pid,process.linux_start_ticks.value.value_or(0)});
            });
            auto report=discover_startup_process_candidates(current);
            if(!report.complete || report.error!=ActionError::none)++partial_scans;
            unavailable+=report.unavailable;
            for(const auto& candidate:report.candidates) {
                if(candidate.baseline.native_context!=context)throw std::runtime_error("context changed");
                const auto result=observer.register_process(candidate.startup_entry_id,candidate.baseline);
                if(result)registered.emplace(candidate.baseline.pid,candidate.baseline.linux_start_ticks);
                else ++rejected;
            }
            observer.poll();
        };
        scan(std::move(snapshot));
        // Start the phase AFTER the first discovery: initial candidates receive
        // a whole window, late arrivals remain explicitly partial at phase end.
        const auto started=std::chrono::steady_clock::now();
        if(started.time_since_epoch().count()>std::numeric_limits<std::chrono::steady_clock::duration::rep>::max()-native_window.count()) {
            std::cerr<<"Capture deadline overflow.\n";return 3;
        }
        const auto deadline=started+native_window;
        while(std::chrono::steady_clock::now()<deadline) {
            std::this_thread::sleep_until(std::min(deadline,std::chrono::steady_clock::now()+250ms));
            if(std::chrono::steady_clock::now()>=deadline)break;
            scan(monitor.sample(options));
        }
        observer.poll(); // final attempts before closing records with shorter coverage
        observer.close();
        StartupImpactArchive archive{capture_id,context,std::chrono::system_clock::now(),observer.observations(),{}}; // No recorder diagnostics: legacy v1.
        const auto saved=StartupImpactStore::save(path,archive);
        if(!saved){std::cerr<<(saved.replaced?"Capture published but durability confirmation failed; no rollback.\n":"Capture save failed.\n");return 4;}
        const auto complete=std::count_if(archive.observations.begin(),archive.observations.end(),[](const auto& row){return row.window_complete;});
        std::cout<<"Historical capture saved: "<<archive.observations.size()<<" associated process observations, "<<complete<<" complete observed windows.\n"
            <<"Partial scans: "<<partial_scans<<", unavailable attempts: "<<unavailable<<", rejected registrations: "<<rejected<<".\n"
            <<"Estimated associations; automatic startup origin NOT certified. Empty capture is not zero startup impact.\n";
        return 0;
    } catch(...) {
        std::cerr<<"Capture failed; no successful completion claimed.\n";return 5;
    }
#endif
}
