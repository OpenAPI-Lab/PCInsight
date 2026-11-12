#include <pcinsight/sdk_headers.hpp>

void check_peer();

int main() {
    pcinsight::require_matching_sdk_headers();
    check_peer();
}
