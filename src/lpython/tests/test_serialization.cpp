#include <tests/doctest.h>
#include <iostream>

#include <libasr/bwriter.h>
#include <libasr/serialization.h>
#include <lpython/ast_serialization.h>
#include <libasr/modfile.h>
#include <lpython/pickle.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>

using LFortran::TRY;
using LFortran::string_to_uint64;
using LFortran::uint64_to_string;
using LFortran::string_to_uint32;
using LFortran::uint32_to_string;

TEST_CASE("Integer conversion") {
    uint64_t i;
    i = 1;
    CHECK(string_to_uint32(uint32_to_string(i)) == i);
    CHECK(string_to_uint64(uint64_to_string(i)) == i);

    i = 150;
    CHECK(string_to_uint32(uint32_to_string(i)) == i);
    CHECK(string_to_uint64(uint64_to_string(i)) == i);

    i = 256;
    CHECK(string_to_uint32(uint32_to_string(i)) == i);
    CHECK(string_to_uint64(uint64_to_string(i)) == i);

    i = 65537;
    CHECK(string_to_uint32(uint32_to_string(i)) == i);
    CHECK(string_to_uint64(uint64_to_string(i)) == i);

    i = 16777217;
    CHECK(string_to_uint32(uint32_to_string(i)) == i);
    CHECK(string_to_uint64(uint64_to_string(i)) == i);

    i = 4294967295LU;
    CHECK(string_to_uint32(uint32_to_string(i)) == i);
    CHECK(string_to_uint64(uint64_to_string(i)) == i);

    i = 4294967296LU;
    CHECK(string_to_uint32(uint32_to_string(i)) != i);
    CHECK(string_to_uint64(uint64_to_string(i)) == i);

    i = 18446744073709551615LLU;
    CHECK(string_to_uint32(uint32_to_string(i)) != i);
    CHECK(string_to_uint64(uint64_to_string(i)) == i);
}

TEST_CASE("Topological sorting int") {
    std::map<int, std::vector<int>> deps;
    // 1 depends on 2
    deps[1].push_back(2);
    // 3 depends on 1, etc.
    deps[3].push_back(1);
    deps[2].push_back(4);
    deps[3].push_back(4);
    CHECK(LFortran::ASRUtils::order_deps(deps) == std::vector<int>({4, 2, 1, 3}));

    deps.clear();
    deps[1].push_back(2);
    deps[1].push_back(3);
    deps[2].push_back(4);
    deps[3].push_back(4);
    CHECK(LFortran::ASRUtils::order_deps(deps) == std::vector<int>({4, 2, 3, 1}));

    deps.clear();
    deps[1].push_back(2);
    deps[3].push_back(1);
    deps[3].push_back(4);
    deps[4].push_back(1);
    CHECK(LFortran::ASRUtils::order_deps(deps) == std::vector<int>({2, 1, 4, 3}));
}

TEST_CASE("Topological sorting string") {
    std::map<std::string, std::vector<std::string>> deps;
    // A depends on B
    deps["A"].push_back("B");
    // C depends on A, etc.
    deps["C"].push_back("A");
    deps["B"].push_back("D");
    deps["C"].push_back("D");
    CHECK(LFortran::ASRUtils::order_deps(deps) == std::vector<std::string>(
                {"D", "B", "A", "C"}));

    deps.clear();
    deps["module_a"].push_back("module_b");
    deps["module_c"].push_back("module_a");
    deps["module_c"].push_back("module_d");
    deps["module_d"].push_back("module_a");
    CHECK(LFortran::ASRUtils::order_deps(deps) == std::vector<std::string>(
                {"module_b", "module_a", "module_d", "module_c"}));
}
