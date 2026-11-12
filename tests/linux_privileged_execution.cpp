#include "../src/linux_privileged_execution.hpp"
#include <cassert>
using namespace pcinsight::detail;
int main(){
    PrivilegedExecutionGate gate;assert(!gate.enabled&&!gate.permit_interaction&&gate.manager_uid==UINT32_MAX&&!gate.busy.test());
    assert(!decode_native_privileged_request(nullptr,PrivilegedRequestKind::control).value);
    assert(!decode_native_privileged_request(nullptr,static_cast<PrivilegedRequestKind>(77)).value);
}
