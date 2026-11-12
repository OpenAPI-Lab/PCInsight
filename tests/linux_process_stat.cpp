#include "../src/linux_process_stat.hpp"
#include <cassert>
#include <limits>
#include <string>
#include <array>
using namespace pcinsight::detail;
namespace {
std::string fixture(std::string user="0", std::string kernel="0", std::string start="100") {
    return "123 (name ) with (spaces\n) S 12 0 0 0 -1 0 0 0 0 0 " + user + " " + kernel +
        " 0 0 20 0 3 0 " + start + " 12345 0 0";
}
}
int main() {
    auto zero=parse_linux_process_stat(fixture());
    assert(zero && zero->pid==123 && zero->parent_pid==12 && zero->threads==3);
    assert(zero->ticks==0 && zero->start_ticks==100);
    auto current=parse_linux_process_stat(fixture("5","7")); assert(current && current->ticks==12);
    assert(current->user_ticks==5&&current->kernel_ticks==7);
    const auto masked_reset=parse_linux_process_stat(fixture("4","20"));assert(masked_reset&&!linux_process_tick_delta(*current,*masked_reset));
    assert(!parse_linux_process_stat(fixture()+std::string(1,'\0')));
    auto zero_pid=fixture();zero_pid.replace(0,3,"0");assert(!parse_linux_process_stat(zero_pid));
    assert(linux_process_tick_delta(*zero,*current)==12);
    assert(linux_process_tick_delta(*zero,*zero)==0);
    auto reused=*current; reused.start_ticks=101;
    assert(!linux_process_tick_delta(*zero,reused));
    reused=*current; reused.pid=124; assert(!linux_process_tick_delta(*zero,reused));
    assert(!linux_process_tick_delta(*current,*zero));
    for (const auto& invalid : {std::string{}, std::string("123 no parentheses"),
        std::string("123 (name) S 12"), fixture("-1"), fixture("1bad"),
        fixture("18446744073709551616"), fixture("18446744073709551615","1"),
        fixture("0","0","bad"), std::string(65537,'x')})
        assert(!parse_linux_process_stat(invalid));
    auto maximum=parse_linux_process_stat(fixture("18446744073709551615"));
    assert(maximum && maximum->ticks==std::numeric_limits<std::uint64_t>::max());
    auto no_newline=fixture(); assert(parse_linux_process_stat(no_newline));
    assert(parse_linux_process_stat(no_newline+"\n"));
    assert(parse_linux_process_io("read_bytes: 0\nwrite_bytes: 0\n") == 0);
    assert(parse_linux_process_io("rchar: 99999\nwrite_bytes: 7\nread_bytes: 5\n") == 12);
    for (const auto& input : {"", "read_bytes: 0\n", "read_bytes: -1\nwrite_bytes: 0\n",
        "read_bytes: 1\nwrite_bytes: bad\n", "read_bytes: 1 trailing\nwrite_bytes: 0\n",
        "read_bytes: 1\nread_bytes: 2\nwrite_bytes: 0\n",
        "read_bytes: 18446744073709551615\nwrite_bytes: 1\n"})
        assert(!parse_linux_process_io(input));
    std::array<unsigned char,20> header{0x7f,'E','L','F',2,1,1};
    header[18]=62; assert(linux_elf_architecture(header)=="x64");
    header[18]=183; assert(linux_elf_architecture(header)=="ARM64");
    header[18]=243; assert(linux_elf_architecture(header)=="RISC-V64");
    header[4]=1; assert(linux_elf_architecture(header)=="RISC-V32");
    header[18]=40; assert(linux_elf_architecture(header)=="ARM32");
    header[18]=3; assert(linux_elf_architecture(header)=="x86");
    header[4]=2; assert(linux_elf_architecture(header).empty());
    header[5]=2; header[18]=0; header[19]=21;
    assert(linux_elf_architecture(header)=="PowerPC64");
    header[4]=1; header[19]=20; assert(linux_elf_architecture(header)=="PowerPC32");
    header[19]=255; assert(linux_elf_architecture(header).empty());
    header[5]=0; assert(linux_elf_architecture(header).empty());
    header[5]=1; header[6]=0; assert(linux_elf_architecture(header).empty());
    assert(linux_elf_architecture(std::span(header).first(19)).empty());
}
