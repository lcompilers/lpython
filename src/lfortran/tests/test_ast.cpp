#include <tests/doctest.h>

#include <iostream>

#include <lfortran/parser/alloc.h>
#include <lfortran/casts.h>
#include <lfortran/ast.h>
#include <lfortran/asr.h>

namespace LFortran {

TEST_CASE("Operator types") {
    std::cout << "OK: " << AST::operatorType::Pow  << std::endl;
    std::cout << "OK: " << ASR::operatorType::Pow  << std::endl;
}

template <class T, class U>
inline bool is_a2(const U &x)
{
    return T::class_type == x.type;
}

template <class T>
static inline T* down_cast2(const AST::ast_t *f)
{
    typedef typename T::parent_type ptype;
    LFORTRAN_ASSERT(is_a2<ptype>(*f));
    ptype *t = (ptype *)f;
    LFORTRAN_ASSERT(is_a2<T>(*t));
    return (T*)t;
}


TEST_CASE("Test types") {
    Allocator al(1024*1024);
    Location loc;

    AST::ast_t &a = *AST::make_Num_t(al, loc, 5);
    CHECK(is_a2<AST::expr_t>(a));
    CHECK(! is_a2<AST::stmt_t>(a));

    AST::Num_t &x = *down_cast2<AST::Num_t>(&a);
    CHECK(AST::is_a<AST::Num_t>(x));
    CHECK(! AST::is_a<AST::BinOp_t>(x));

}

} // namespace LFortran
