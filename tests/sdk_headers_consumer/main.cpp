#include <pcinsight/sdk_headers.hpp>
int main() {
    static_assert(sizeof(pcinsight::sdk_headers_fingerprint) == 65);
    pcinsight::require_matching_sdk_headers();
}
