#include "../src/linux_network_join.hpp"
#include <cassert>
using namespace pcinsight;using namespace pcinsight::detail;
int main(){
    PrivateNetworkProcContext c{{1,2},{7,42,100,1,2,3,4,5,6,7},{8,43,200,1,2,8,4,9,6,10}};
    PrivateNetworkIdentity kernel{999,1000,123456};
    auto join=[&](const auto& b,const auto& k,const auto& a){return validate_private_pidfd_network_join(8,200,b,kernel,k,a);};
    // Local PID/UID/start and target namespace deliberately differ from kernel
    // identifiers and observer namespace. No equality/conversion assumption.
    auto good=validate_private_pidfd_network_join(8,200,c,kernel,kernel,c);
    assert(!private_network_join_is_self(c));
    auto self=c;self.process=self.observer;assert(private_network_join_is_self(self));
    for(unsigned field=0;field<10;++field){auto bad=self;
        if(field==0)++bad.process.local_pid;if(field==1)++bad.process.local_real_uid;if(field==2)++bad.process.start_ticks;
        if(field==3)++bad.process.procfs_device;if(field==4)++bad.process.procfs_inode;if(field==5)++bad.process.process_inode;
        if(field==6)++bad.process.pid_namespace_device;if(field==7)++bad.process.pid_namespace_inode;
        if(field==8)++bad.process.user_namespace_device;if(field==9)++bad.process.user_namespace_inode;
        assert(!private_network_join_is_self(bad));}
    auto invalid_self=self;invalid_self.boot={};assert(!private_network_join_is_self(invalid_self));
    invalid_self=self;invalid_self.observer.start_ticks=invalid_self.process.start_ticks=0;assert(!private_network_join_is_self(invalid_self));
    assert(good.value&&good.value->kernel.kernel_tgid==999&&good.value->local.process.local_real_uid==43);
    for(unsigned i=0;i<17;++i){auto changed=c;
        switch(i){
        case 0:++changed.boot[0];break;case 1:++changed.boot[1];break;
        case 2:++changed.observer.local_pid;break;case 3:++changed.observer.local_real_uid;break;
        case 4:++changed.observer.start_ticks;break;case 5:++changed.observer.procfs_device;break;
        case 6:++changed.observer.procfs_inode;break;case 7:++changed.observer.process_inode;break;
        case 8:++changed.observer.pid_namespace_device;break;case 9:++changed.observer.pid_namespace_inode;break;
        case 10:++changed.observer.user_namespace_device;break;case 11:++changed.observer.user_namespace_inode;break;
        case 12:++changed.process.local_pid;break;case 13:++changed.process.local_real_uid;break;
        case 14:++changed.process.start_ticks;break;case 15:++changed.process.process_inode;break;
        case 16:++changed.process.user_namespace_inode;break;
        }assert(!join(c,kernel,changed).value);
    }
    for(unsigned i=0;i<3;++i){auto k=kernel;if(i==0)++k.kernel_tgid;if(i==1)++k.kernel_real_uid;if(i==2)++k.leader_start_boottime_ns;assert(!join(c,k,c).value);}
    for(unsigned i=0;i<9;++i){auto b=c;auto k=kernel;std::uint32_t pid=8;std::uint64_t start=200;
        if(i==0)b.boot={};if(i==1)b.observer.start_ticks=0;if(i==2)b.process.process_inode=0;
        if(i==3)b.process.procfs_device=2;if(i==4)b.process.procfs_inode=3;
        if(i==5)k.kernel_tgid=0;if(i==6)k.leader_start_boottime_ns=0;if(i==7)pid=9;if(i==8)start=201;
        assert(!validate_private_pidfd_network_join(pid,start,b,k,k,b).value);
    }
    kernel.kernel_real_uid=0;c.process.local_real_uid=0;assert(validate_private_pidfd_network_join(8,200,c,kernel,kernel,c).value);
    assert(!read_private_pidfd_network_join(false,0,8,200).value);
    assert(!read_private_pidfd_network_join(true,-1,8,200).value);
    assert(!read_private_pidfd_network_join(true,0,0,200).value);
    assert(!read_private_pidfd_network_join(true,0,8,0).value);
    assert(!read_linux_network_pidfd_process(-1,8,200).value);
}
