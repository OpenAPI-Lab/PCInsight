#include "../src/linux_network_self.hpp"
#include <cassert>
#include <iostream>
#include <thread>
#ifdef __linux__
#include <dirent.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif
using namespace pcinsight;using namespace pcinsight::detail;
int main(int argc,char** argv){
    const bool selected=argc==2&&std::string_view(argv[1])=="--native-selected";
    if(selected||(argc==2&&std::string_view(argv[1])=="--native")){
        auto read=read_linux_network_self();if(!read.value){std::cerr<<"UNAVAILABLE: "<<read.note<<'\n';return 77;}
#ifdef __linux__
        auto count=[](){auto* d=::opendir("/proc/self/fd");assert(d);unsigned n=0;while(auto* e=::readdir(d))if(e->d_name[0]!='.')++n;::closedir(d);return n;};
        const auto initial=count();for(unsigned i=0;i<20;++i){auto again=read_linux_network_self();assert(again.value&&*again.value==*read.value);}
        if(selected){
            int pipes[2]{-1,-1};assert(::pipe2(pipes,O_CLOEXEC)==0);bool unavailable=false;
            std::thread worker([&]{
                if(::unshare(CLONE_FILES)<0){unavailable=true;return;}
                assert(::close(pipes[0])==0);
                const int pidfd=static_cast<int>(::syscall(SYS_pidfd_open,::getpid(),0));
                if(pidfd<0){unavailable=true;return;}
                assert(pidfd==pipes[0]); // Leader's same-numbered FD remains a pipe.
                auto target=read_linux_network_pidfd_process(pidfd,read.value->local_pid,read.value->start_ticks);
                assert(target.value&&*target.value==*read.value);
                assert(!read_linux_network_pidfd_process(pidfd,read.value->local_pid,read.value->start_ticks+1).value);
                assert(!read_linux_network_pidfd_process(pipes[1],read.value->local_pid,read.value->start_ticks).value);
                assert(::close(pidfd)==0);
            });worker.join();
            assert(::fcntl(pipes[0],F_GETFD)>=0);assert(::close(pipes[0])==0);assert(::close(pipes[1])==0);
            assert(count()==initial);
            if(unavailable){std::cerr<<"UNAVAILABLE selected native: unshare(CLONE_FILES)/pidfd_open\n";return 77;}
            std::cout<<"PASS selected procfs caller-thread FD table: same descriptor is leader pipe/worker pidfd; stale start and pipe rejected; no FD leak\n";
        }
        assert(count()==initial);assert(read.value->local_pid==static_cast<std::uint32_t>(::getpid())&&read.value->local_real_uid==static_cast<std::uint32_t>(::getuid()));
        std::cout<<"PASS procfs SELF pid="<<read.value->local_pid<<" start_ticks="<<read.value->start_ticks<<" uid="<<read.value->local_real_uid
            <<" pidns="<<read.value->pid_namespace_inode<<" userns="<<read.value->user_namespace_inode<<"; stable20; no FD leak\n";
#endif
        return 0;
    }
    const std::string good="Name:\tself\nTgid:\t7\nPid:\t7\nUid:\t0 1 2 4294967295\nNStgid:\t999 7\n";
    auto r=parse_network_self_status(good);assert(r.value&&r.value->pid==7&&r.value->tgid==7&&r.value->real_uid==0);
    for(auto text:{"", "Pid: 7\nTgid: 7\n", "Pid: 0\nTgid: 7\nUid: 0 0 0 0\n", "Pid: 2147483648\nTgid: 7\nUid: 0 0 0 0\n",
        "Pid: 7\nTgid: 7\nUid: 0 0 0\n", "Pid: 7 8\nTgid: 7\nUid: 0 0 0 0\n", "Pid: -7\nTgid: 7\nUid: 0 0 0 0\n",
        "Pid: 7\nTgid: 7\nUid: 4294967296 0 0 0\n", "Pid: 7\nTgid: 7\nUid: 0 0 0 0 0\n"})assert(!parse_network_self_status(text).value);
    for(auto duplicate:{"Pid: 7\n","Tgid: 7\n","Uid: 0 0 0 0\n"})assert(!parse_network_self_status(good+duplicate).value);
    assert(!parse_network_self_status(std::string(65537,'x')).value);assert(!parse_network_self_status(good+std::string(1,'\0')).value);
    LinuxNetworkSelfIdentity identity{7,0,100,1,1,7,2,3,2,4};assert(valid_network_self(identity));
    for(unsigned field=0;field<6;++field){auto bad=identity;if(field==0)bad.local_pid=0;if(field==1)bad.start_ticks=0;if(field==2)bad.procfs_inode=0;
        if(field==3)bad.process_inode=0;if(field==4)bad.pid_namespace_inode=0;if(field==5)bad.user_namespace_inode=0;assert(!valid_network_self(bad));}
#ifndef __linux__
    assert(!read_linux_network_self().value);
#endif
}
