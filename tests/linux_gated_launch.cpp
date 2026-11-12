#include "../src/linux_gated_launch.hpp"
#include "../src/linux_network_join.hpp"
#include "../src/linux_selected_exec.hpp"
#include "../src/linux_launch.hpp"
#include <cassert>
#include <iostream>
#include <string_view>
#ifdef __linux__
#include <filesystem>
#include <fstream>
#include <iterator>
#include <dirent.h>
#include <cerrno>
#include <cstdlib>
#include <pthread.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <unistd.h>
using namespace pcinsight;using namespace pcinsight::detail;namespace fs=std::filesystem;
std::size_t fds(){auto d=::opendir("/proc/self/fd");assert(d);std::size_t n=0;while(auto e=::readdir(d))if(e->d_name[0]!='.')++n;::closedir(d);return n;}
std::string read(const fs::path& p){std::ifstream f(p);return {std::istreambuf_iterator<char>(f),{}};}
int main(int argc,char** argv){
    if(argc==4&&std::string_view(argv[1])=="--reexec"){::execl(argv[2],"pcinsight-private-reexec","--child",argv[3],static_cast<char*>(nullptr));return 123;}
    if(argc>=3&&std::string_view(argv[1])=="--child"){
        struct stat info{};if(::stat("/proc/self/exe",&info)!=0)return 120;
        std::ofstream f(argv[2]);f<<"NNP="<<::prctl(PR_GET_NO_NEW_PRIVS,0,0,0,0)<<" inode="<<info.st_ino<<'\n';for(int n=3;n<argc;++n)f<<std::string_view(argv[n]).size()<<':'<<argv[n]<<'\n';return f?7:121;
    }
    const bool exec_check=argc==2&&std::string_view(argv[1])=="--authorized-private-selected-exec";
    const bool kernel_check=exec_check||(argc==2&&std::string_view(argv[1])=="--authorized-private-gated");
    assert(argc==1||kernel_check);::alarm(exec_check?60:25);const auto original_fds=fds();const auto nnp=::prctl(PR_GET_NO_NEW_PRIVS,0,0,0,0);sigset_t original_mask{};assert(::pthread_sigmask(SIG_SETMASK,nullptr,&original_mask)==0);
    char pattern[]="/tmp/pcinsight-gated-launch-XXXXXX";const auto d=::mkdtemp(pattern);assert(d);const fs::path root=d;
    struct Cleanup{fs::path p;~Cleanup(){std::error_code e;fs::remove_all(p,e);}} cleanup{root};
    const auto image=fs::read_symlink("/proc/self/exe").string(),out=(root/"output").string();
    LaunchRequest req{image,{"--child",out,"","two words","$(touch NO)","a; touch NO"},root.string(),false};
    unsigned calls=0;std::uint32_t observed=0;std::uint64_t inode=0;
    LinuxSelectedExecCapture exec_capture;
    auto arm=[&](const LinuxGatedLaunchTarget& t){++calls;observed=t.child.local_pid;inode=t.executable.inode;struct stat s{};
        assert(t.borrowed_pidfd>=0&&t.borrowed_executable_fd>=0&&t.child.start_ticks&&t.child.pid_namespace_inode==t.owner.observer.pid_namespace_inode&&t.child.local_pid!=t.owner.observer.local_pid);
        assert(::fstat(t.borrowed_executable_fd,&s)==0&&static_cast<std::uint64_t>(s.st_ino)==inode&&!fs::exists(out));
        if(kernel_check){const auto joined=read_private_pidfd_network_join(true,t.borrowed_pidfd,t.child.local_pid,t.child.start_ticks);
            if(!joined.value)std::cerr<<"private gated kernel join unavailable: "<<joined.note<<'\n';
            assert(joined.value&&joined.support==Support::available&&joined.value->local.boot==t.owner.boot&&joined.value->local.observer==t.owner.observer&&joined.value->local.process==t.child&&!fs::exists(out));
            std::cout<<"gated selected identity: TGID="<<joined.value->kernel.kernel_tgid<<" start_boottime_ns="<<joined.value->kernel.leader_start_boottime_ns<<" BEFORE exec release; temporary private query removed\n";
        }
        if(exec_check){const auto started=exec_capture.start(true,t);if(!started.value)std::cerr<<"selected exec start: "<<started.note<<'\n';assert(started.value&&started.support==Support::available);
            const auto sentinel_path=(root/"sentinel").string();const auto sentinel=launch_linux_process({image,{"--child",sentinel_path},root.string(),false});
            int status{};assert(sentinel&&::waitpid(static_cast<pid_t>(sentinel.process_id),&status,0)==static_cast<pid_t>(sentinel.process_id)&&WIFEXITED(status)&&WEXITSTATUS(status)==7);fs::remove(sentinel_path);
            const auto pending=exec_capture.sample();assert(pending.value&&!pending.value->evidence.first_exec.value&&!pending.value->evidence.pinned_file_matches.value&&!pending.value->evidence.quality_flags&&!pending.value->pidfd_exited&&!fs::exists(out));
        }
        return ActionResult{};};
    auto verify_exec=[&](bool expected){if(!exec_check)return;const auto first=exec_capture.sample();if(!first.value)std::cerr<<"selected exec sample: "<<first.note<<'\n';
        assert(first.value&&first.value->pidfd_exited&&!first.value->evidence.publishing&&!first.value->evidence.quality_flags);
        if(expected)assert(first.value->evidence.first_exec.value&&first.value->evidence.first_exec.value->inode==inode&&first.value->evidence.pinned_file_matches.value==true);
        else assert(!first.value->evidence.first_exec.value&&!first.value->evidence.pinned_file_matches.value);
        const auto replay=exec_capture.sample();assert(replay.value&&replay.value->evidence.first_exec.value==first.value->evidence.first_exec.value&&replay.value->evidence.pinned_file_matches.value==first.value->evidence.pinned_file_matches.value);
        std::cout<<"selected successful exec: "<<(expected?"known pinned match":"absent remains UNKNOWN")<<" AFTER reap; unselected exec ignored/own replay immutable\n";exec_capture.stop();assert(!exec_capture.sample().value);
    };
    assert(launch_linux_gated(req,false,arm).error==ActionError::permission_denied&&!calls&&!fs::exists(out));
    auto result=launch_linux_gated(req,true,arm);
    if(result.error==ActionError::not_supported&&!calls){assert(!result.process_id&&!fs::exists(out)&&fds()==original_fds);std::cout<<"gated launch unavailable (clone3/openat2), safe no-child/no-fallback contract passed; POSITIVE launch NOT verified\n";return 0;}
    if(!result)std::cerr<<"gated launch failed: "<<static_cast<unsigned>(result.error)<<' '<<result.message<<'\n';
    assert(result&&result.process_id==observed&&calls==1);int status{};assert(::waitpid(static_cast<pid_t>(result.process_id),&status,0)==static_cast<pid_t>(result.process_id)&&WIFEXITED(status)&&WEXITSTATUS(status)==7);
    assert(read(out)=="NNP=1 inode="+std::to_string(inode)+"\n0:\n9:two words\n11:$(touch NO)\n11:a; touch NO\n"&&!fs::exists(root/"NO"));
    verify_exec(true);
    fs::remove(out);
    // Denial/exception occurs while gated: no application entry/marker, reaped.
    for(unsigned mode=0;mode<2;++mode){result=launch_linux_gated(req,true,[&](const auto& t)->ActionResult{observed=t.child.local_pid;if(mode)throw 1;return {ActionError::permission_denied,"capture arm denied"};});
        assert(!result&&!result.process_id&&!fs::exists(out));assert(::waitpid(static_cast<pid_t>(observed),&status,WNOHANG)<0&&errno==ECHILD);}
    result=launch_linux_gated(req,true,[&](const auto& t)->ActionResult{observed=t.child.local_pid;assert(::syscall(SYS_pidfd_send_signal,t.borrowed_pidfd,SIGSTOP,nullptr,0)==0);
        int stopped{};assert(::waitpid(static_cast<pid_t>(observed),&stopped,WUNTRACED)==static_cast<pid_t>(observed)&&WIFSTOPPED(stopped));return {ActionError::permission_denied,"cancel stopped private child"};});
    assert(!result&&!result.process_id&&!fs::exists(out)&&::waitpid(static_cast<pid_t>(observed),&status,WNOHANG)<0&&errno==ECHILD);
    // Retargeting an ordinary symlink cannot replace the already pinned image.
    const auto link=root/"program";fs::create_symlink(image,link);auto pinned=req;pinned.executable=link.string();
    result=launch_linux_gated(pinned,true,[&](const auto& t){auto accepted=arm(t);fs::remove(link);fs::create_symlink(root/"missing-program",link);return accepted;});
    assert(result&&::waitpid(static_cast<pid_t>(result.process_id),&status,0)==static_cast<pid_t>(result.process_id)&&WIFEXITED(status)&&WEXITSTATUS(status)==7&&fs::exists(out));verify_exec(true);fs::remove(out);
    auto missing=req;missing.working_directory=(root/"missing-cwd").string();result=launch_linux_gated(missing,true,arm);assert(!result&&!result.process_id&&!fs::exists(out));verify_exec(false);
    auto relative=req;relative.executable="program";assert(launch_linux_gated(relative,true,arm).error==ActionError::invalid_argument);
    auto elevated=req;elevated.elevated=true;assert(launch_linux_gated(elevated,true,arm).error==ActionError::permission_denied);
    auto magic=req;magic.executable="/proc/self/exe";assert(!launch_linux_gated(magic,true,arm)&&!fs::exists(out));
    const auto script=root/"script";{std::ofstream f(script);f<<"#!/bin/sh\ntouch NO\n";}::chmod(script.c_str(),0700);auto shell=req;shell.executable=script.string();assert(!launch_linux_gated(shell,true,arm)&&!fs::exists(root/"NO"));
    const auto fifo=root/"fifo";assert(::mkfifo(fifo.c_str(),0600)==0);auto special=req;special.executable=fifo.string();assert(!launch_linux_gated(special,true,arm));
    const auto broken=root/"broken-elf";{std::ofstream f(broken,std::ios::binary);f<<"\177ELFnot-a-real-program";}assert(::chmod(broken.c_str(),0700)==0);auto bad_elf=req;bad_elf.executable=broken.string();
    result=launch_linux_gated(bad_elf,true,arm);assert(!result&&!result.process_id&&!fs::exists(out)); // Real exec error, no shell fallback.
    verify_exec(false);
    if(exec_check){const auto next=root/"second-image";fs::copy_file(image,next);assert(::chmod(next.c_str(),0700)==0);struct stat other{};assert(::stat(next.c_str(),&other)==0);
        auto reexec=req;reexec.arguments={"--reexec",next.string(),out};result=launch_linux_gated(reexec,true,arm);assert(result&&::waitpid(static_cast<pid_t>(result.process_id),&status,0)==static_cast<pid_t>(result.process_id)&&WIFEXITED(status)&&WEXITSTATUS(status)==7);
        assert(static_cast<std::uint64_t>(other.st_ino)!=inode&&read(out)=="NNP=1 inode="+std::to_string(other.st_ino)+"\n");verify_exec(true);fs::remove(out);
        std::cout<<"selected first exec survives re-exec to DIFFERENT inode/device; no current-image inference\n";
    }
    sigset_t final_mask{};assert(::pthread_sigmask(SIG_SETMASK,nullptr,&final_mask)==0);for(int sig=1;sig<NSIG;++sig)assert(::sigismember(&final_mask,sig)==::sigismember(&original_mask,sig));
    assert(fds()==original_fds&&::prctl(PR_GET_NO_NEW_PRIVS,0,0,0,0)==nnp);::alarm(0);
    std::cout<<"gated launch POSITIVE: pre-exec identity/arming/pinned ELF/symlink/literal argv/NNP/deny/throw/failure/reap/FD cleanup passed; exec receipt/cause NOT certified\n";
}
#else
int main(){using namespace pcinsight;using namespace pcinsight::detail;assert(launch_linux_gated({},false,{}).error==ActionError::permission_denied);assert(launch_linux_gated({},true,{}).error==ActionError::not_supported);}
#endif
