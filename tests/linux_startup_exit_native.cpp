// Manual, authorized private-process test ONLY. Observer is NOT target's parent.
#include "../src/linux_startup_exit.hpp"
#include "../src/linux_process_details.hpp"
#include <cerrno>
#include <csignal>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <dirent.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>
using namespace pcinsight;using namespace pcinsight::detail;using namespace std::chrono_literals;
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void transfer(int fd,void* data,std::size_t count,bool write){auto p=static_cast<char*>(data);while(count){const auto n=write?::write(fd,p,count) : ::read(fd,p,count);if(n<0&&errno==EINTR)continue;check(n>0,"private IPC");p+=n;count-=static_cast<std::size_t>(n);}}
std::size_t descriptors(){auto d=::opendir("/proc/self/fd");check(d,"FD census");std::size_t n=0;while(const auto e=::readdir(d))if(e->d_name[0]!='.')++n;::closedir(d);return n;}
struct Guard {pid_t parent=-1;int go[2]{-1,-1},out[2]{-1,-1},pidfd=-1;~Guard(){if(go[1]>=0){char c='x';const auto sent=::write(go[1],&c,1);(void)sent;}if(parent>0){::kill(parent,SIGKILL);while(::waitpid(parent,nullptr,0)<0&&errno==EINTR){}}for(int f:{go[0],go[1],out[0],out[1],pidfd})if(f>=0)::close(f);}};
void run(bool threaded){::alarm(25);::signal(SIGPIPE,SIG_IGN);::signal(SIGCHLD,SIG_DFL);const auto base=descriptors();{
    Guard g;check(::pipe2(g.go,O_CLOEXEC)==0&&::pipe2(g.out,O_CLOEXEC)==0,"private pipes");
    g.parent=::fork();check(g.parent>=0,"owner fork");if(!g.parent){::alarm(20);::close(g.go[1]);::close(g.out[0]);
        const auto target=::fork();if(target<0)::_exit(120);if(!target){
            // Grandchild is private, blocked until observer attaches. It exits
            // without any second user-space sample and is reaped by its owner.
            char command=0;transfer(g.go[0],&command,1,false);
            if(threaded){check(command=='L',"leader exit command");const auto input=g.go[0],output=g.out[1];
                std::thread([input,output]{
                    const auto deadline=std::chrono::steady_clock::now()+3s;bool zombie=false;
                    while(std::chrono::steady_clock::now()<deadline){std::ifstream file("/proc/self/stat");std::string stat;std::getline(file,stat);const auto end=stat.rfind(')');
                        if(end!=stat.npos&&end+2<stat.size()&&stat[end+2]=='Z'){zombie=true;break;}std::this_thread::sleep_for(1ms);}
                    if(!zombie)::_exit(116);char ready='T';transfer(output,&ready,1,true);char finish=0;transfer(input,&finish,1,false);::_exit(finish=='G'?7:117);
                }).detach();
                // Linux thread-only exit, deliberately no C++ stack unwind or
                // exit_group: worker remains live while the leader is zombie.
                ::syscall(SYS_exit,0);::_exit(118);
            }
            ::_exit(command=='G'?7:119);}
        ::close(g.go[0]);auto pid=static_cast<std::uint32_t>(target);transfer(g.out[1],&pid,sizeof(pid),true);int status=0;
        while(::waitpid(target,&status,0)<0&&errno==EINTR){};char ack=WIFEXITED(status)&&WEXITSTATUS(status)==7?'R':'F';transfer(g.out[1],&ack,1,true);::_exit(ack=='R'?0:1);}
    ::close(g.go[0]);g.go[0]=-1;::close(g.out[1]);g.out[1]=-1;
    std::uint32_t pid=0;transfer(g.out[0],&pid,sizeof(pid),false);
    g.pidfd=static_cast<int>(::syscall(SYS_pidfd_open,pid,0));check(g.pidfd>=0,"private pidfd");
    const auto stat=read_linux_process_identity(std::filesystem::path("/proc")/std::to_string(pid));check(stat.value.has_value(),"private stat baseline");
    siginfo_t info{};errno=0;check(::waitid(P_PIDFD,g.pidfd,&info,WEXITED|WNOHANG|WNOWAIT)<0&&errno==ECHILD,"observer unexpectedly owns target child");
    LinuxStartupSelectedExitCapture capture;auto started=capture.start(true,g.pidfd,pid,stat.value->start_ticks);check(started.value.has_value(),started.note.c_str());
    auto read=capture.sample();check(read.value&&!read.value->event&&!read.value->pidfd_exited,"invented initial exit");
    // Unselected private process exits: must leave selected map untouched.
    const auto sentinel=::fork();check(sentinel>=0,"sentinel fork");if(!sentinel)::_exit(0);int status=0;check(::waitpid(sentinel,&status,0)==sentinel,"sentinel wait");
    read=capture.sample();check(read.value&&!read.value->event&&!read.value->pidfd_exited,"captured unselected exit");
    const auto copy=::fork();check(copy>=0,"capture fork check");if(!copy){const bool rejected=!capture.sample().value;capture.stop();::_exit(rejected?0:1);}check(::waitpid(copy,&status,0)==copy&&WIFEXITED(status)&&WEXITSTATUS(status)==0,"forked capture accepted");
    char command='G';
    if(threaded){command='L';transfer(g.go[1],&command,1,true);char ready=0;transfer(g.out[0],&ready,1,false);check(ready=='T',"worker did not confirm leader zombie");
        read=capture.sample();check(read.value&&!read.value->event&&!read.value->pidfd_exited,"leader exit incorrectly marked whole group exited");command='G';}
    transfer(g.go[1],&command,1,true);char ack=0;transfer(g.out[0],&ack,1,false);check(ack=='R',"owner did not reap target");
    // Parent ACK proves reaping has happened BEFORE observer reads the event.
    check(!read_linux_process_identity(std::filesystem::path("/proc")/std::to_string(pid)).value,"private target still in procfs after reap");
    read=capture.sample();check(read.value&&read.value->event&&read.value->pidfd_exited,read.note.c_str());
    const auto event=*read.value->event;check(event.identity.kernel_tgid==started.value->kernel.kernel_tgid&&event.identity.kernel_real_uid==started.value->kernel.kernel_real_uid&&event.identity.leader_start_boottime_ns==started.value->kernel.leader_start_boottime_ns&&event.monotonic_ns,"captured identity/time mismatch");
    const auto again=capture.sample();check(again.value&&again.value->event&&again.value->event->monotonic_ns==event.monotonic_ns&&again.value->pidfd_exited,"exit replay changed event");
    capture.stop();check(!capture.sample().value,"stopped capture still reads event");
    check(::waitpid(g.parent,&status,0)==g.parent&&WIFEXITED(status)&&WEXITSTATUS(status)==0,"owner child exit");g.parent=-1;
    std::cout<<"native private foreign PID="<<pid<<" initial TGID="<<event.identity.kernel_tgid<<" threaded="<<threaded<<" last-thread exit-entry monotonic_ns="<<event.monotonic_ns<<" retained AFTER owner reap; no CPU/disk totals claimed\n";
}check(descriptors()==base,"selected exit FD leak");::alarm(0);}
}
int main(int argc,char** argv){if(argc!=2||std::string_view(argv[1])!="--authorized-private-exit"){std::cerr<<"No capture without manual private authorization\n";return 77;}try{run(false);run(true);return 0;}catch(const std::exception& e){std::cerr<<"FAIL selected exit: "<<e.what()<<'\n';return 1;}}
