#include <tests/doctest.h>
#include <iostream>

#include <lfortran/stacktrace.h>
#include <lfortran/pickle.h>
#include <lfortran/ast_to_src.h>
#include <lfortran/parser/parser.h>


TEST_CASE("Address to line number"){
    Allocator al(4*1024);

    std::vector<std::string> filenames = {"foo", "foo1", "foo2", "foo3", "foo4", "foo5", "bar", "bar1", "bar2", "bar3", "bar4", "bar5", "bar6"};
    std::vector<uint64_t> addresses = {123, 234, 345, 456, 567, 678, 789, 890, 901, 1000, 1100, 1150, 1200};
    std::string filename = "";
    int line_number = -1;

    LFortran::address_to_line_number(filenames, addresses, 500, filename, line_number ); //addresses[8], filename=filenames[7]=bar, line_number=890
    CHECK(filename=="bar1");
    CHECK(line_number==901);

    //when address is not in addresses vector
    LFortran::address_to_line_number(filenames, addresses, 1500, filename="", line_number=-1);
    CHECK(filename=="");
    CHECK(line_number==-1);

}