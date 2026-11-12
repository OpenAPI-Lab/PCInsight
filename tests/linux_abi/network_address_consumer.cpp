#include <pcinsight/linux_network_addresses.hpp>
#include <pcinsight/sdk_headers.hpp>
#include <iostream>
int main() {
    pcinsight::require_matching_sdk_headers();
    const auto report=pcinsight::query_linux_network_addresses();
    if(!pcinsight::metric_is_known(report)) {
        std::cout<<"Address query unavailable: "<<report.note<<'\n';
        return 77;
    }
    for(const auto& row:*report.value)if(!row.interface_index)return 1;
    std::cout<<"Complete address query: "<<report.value->size()<<" interface rows\n";
}
