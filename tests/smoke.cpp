#include "pcinsight/monitor.hpp"
#include "pcinsight/actions.hpp"
#include "pcinsight/sampler.hpp"
#include <cassert>
#include <thread>
int main(){pcinsight::Monitor m;auto s=m.sample();assert(s.captured_at.time_since_epoch().count()>0);auto actions=pcinsight::make_system_actions();assert(actions);auto bad=actions->terminate_process(0);assert(!bad);pcinsight::Sampler sampler(std::chrono::milliseconds(10),2);sampler.start();std::this_thread::sleep_for(std::chrono::milliseconds(35));sampler.stop();assert(!sampler.history().empty());assert(sampler.history().size()<=2);return 0;}
