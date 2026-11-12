#include <iostream>
#include <string_view>

// Test both decoding this UTF-8 source and the execution encoding of narrow
// literals. These bytes must not depend on the Windows user's code page.
constexpr std::string_view source = "é用户";
constexpr std::string_view escaped = "\u00e9\u7528\u6237";
constexpr std::string_view expected = "\xC3\xA9\xE7\x94\xA8\xE6\x88\xB7";
static_assert(source == expected && escaped == expected);
static_assert(std::u32string_view(U"é用户") == U"\u00e9\u7528\u6237");
static_assert(std::wstring_view(L"é用户") == L"\u00e9\u7528\u6237");

int main() {
    std::cout << "Source, narrow execution, wide and Unicode literals retain their expected encoding\n";
}
