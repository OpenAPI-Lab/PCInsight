#include "pcinsight/monitor.hpp"
#include <chrono>
#include <iostream>
#include <thread>

template<class T> void show(const pcinsight::Metric<T>& m){if(m.value)std::cout<<*m.value;else std::cout<<"N/D";}
int main(){pcinsight::Monitor monitor;monitor.sample();std::this_thread::sleep_for(std::chrono::seconds(1));auto s=monitor.sample();std::cout<<s.cpu.vendor<<" "<<s.cpu.model<<"\nCPU: ";show(s.cpu.utilization_percent);std::cout<<"%\nRAM: ";show(s.memory.utilization_percent);std::cout<<"%\nProcesos: "<<s.processes.size()<<"\n";for(std::size_t i=0;i<std::min<std::size_t>(s.processes.size(),10);++i){auto&p=s.processes[i];std::cout<<p.pid<<"\t"<<p.name<<"\t";show(p.cpu_percent);std::cout<<"%\n";}}
