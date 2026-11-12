#include <pcinsight/linux_network_addresses.hpp>
#include <cerrno>
#include <cstddef>
#include <iostream>
#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <unistd.h>

int main() {
    alarm(10);
#if defined(__x86_64__) || defined(__aarch64__)
#ifdef __x86_64__
    constexpr auto architecture=AUDIT_ARCH_X86_64;
#else
    constexpr auto architecture=AUDIT_ARCH_AARCH64;
#endif
    sock_filter filter[]={
        BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(seccomp_data,arch)),
        BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,architecture,1,0),
        BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_KILL_PROCESS),
        BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(seccomp_data,nr)),
#ifdef __x86_64__
        BPF_STMT(BPF_ALU|BPF_AND|BPF_K,~0x40000000U),
#endif
        BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,__NR_socket,0,1),
        BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM),
        BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW)
    };
    sock_fprog program{static_cast<unsigned short>(sizeof filter/sizeof *filter),filter};
    if(prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0)!=0||prctl(PR_SET_SECCOMP,SECCOMP_MODE_FILTER,&program)!=0){
        std::cout<<"Seccomp unavailable; permission propagation not verified\n";
        return 77;
    }
    // Calibrate the forced denial; no external network traffic is generated.
    const auto fd=socket(AF_INET,SOCK_DGRAM,0);
    if(fd>=0){close(fd);return 1;}
    if(errno!=EPERM)return 1;
    const auto report=pcinsight::query_linux_network_addresses();
    if(report.value||report.support!=pcinsight::Support::permission_required||
       report.note.find("Create rtnetlink socket")==std::string::npos)return 1;
    std::cout<<"Public address query preserved forced socket EPERM without a value\n";
    return 0;
#else
    std::cout<<"Seccomp test architecture unsupported\n";
    return 77;
#endif
}
