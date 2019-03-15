from lfortran.ast import src_to_ast
from lfortran.asr.asr_check import verify_asr
from lfortran.semantic.ast_to_asr import ast_to_asr
from lfortran.codegen.asr_to_llvm import asr_to_llvm
from lfortran.codegen.evaluator import LLVMEvaluator

def test_function1():
    source = """\
integer function fn1(a, b) result(r)
integer, intent(in) :: a, b
r = a + b
end function
"""
    ast = src_to_ast(source, translation_unit=False)
    asrepr = ast_to_asr(ast)
    verify_asr(asrepr)

    assert 'fn1' in asrepr.global_scope.symbols
    fn1 = asrepr.global_scope.symbols['fn1']
    assert fn1.args[0].name == "a"
    assert fn1.args[1].name == "b"
    assert fn1.return_var.name == "r"
    assert fn1.body[0].target == fn1.return_var
    assert fn1.body[0].value.left == fn1.args[0]
    assert fn1.body[0].value.right == fn1.args[1]

    llmod = asr_to_llvm(asrepr)
    e = LLVMEvaluator()
    e.add_module(str(llmod))
    e.add_module("""\
declare i64 @fn1(i64* %".1", i64* %".2")

define i64 @f1()
{
    %a = alloca i64
    %b = alloca i64
    store i64 2, i64* %a
    store i64 3, i64* %b
    %r = call i64 @fn1(i64* %a, i64* %b)
    ret i64 %r
}
""")
    assert e.intfn("f1") == 5
