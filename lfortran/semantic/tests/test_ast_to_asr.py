import pytest

from lfortran.semantic.ast_to_asr import ast_to_asr
from lfortran.ast import src_to_ast
from lfortran.asr import asr
from lfortran.asr.asr_check import verify_asr

from lfortran.ast.ast_to_src import ast_to_src
from lfortran.asr.asr_to_ast import asr_to_ast


def test_function1():
    source = """\
module test
implicit none
contains

    integer function fn1(a, b) result(r)
    integer, intent(in) :: a, b
    r = a + b
    end function

end module
"""
    ast = src_to_ast(source)
    asrepr = ast_to_asr(ast)
    verify_asr(asrepr)
    assert isinstance(asrepr, asr.TranslationUnit)
    assert 'test' in asrepr.global_scope.symbols
    m = asrepr.global_scope.symbols['test']
    assert isinstance(m, asr.Module)
    fn1 = m.symtab.symbols["fn1"]
    assert isinstance(fn1, asr.Function)
    assert fn1.args[0].name == "a"
    assert fn1.args[1].name == "b"
    assert fn1.return_var.name == "r"
    assert fn1.body[0].target == fn1.return_var
    assert fn1.body[0].value.left == fn1.args[0]
    assert fn1.body[0].value.right == fn1.args[1]

def test_function2():
    source = """\
integer function fn1(a, b) result(r)
integer, intent(in) :: a, b
r = a + b
end function
"""
    ast = src_to_ast(source, translation_unit=False)
    asrepr = ast_to_asr(ast)
    verify_asr(asrepr)
    assert isinstance(asrepr, asr.TranslationUnit)
    assert 'fn1' in asrepr.global_scope.symbols
    fn1 = asrepr.global_scope.symbols['fn1']
    assert isinstance(fn1, asr.Function)
    assert fn1.args[0].name == "a"
    assert fn1.args[1].name == "b"
    assert fn1.return_var.name == "r"
    assert fn1.body[0].target == fn1.return_var
    assert fn1.body[0].value.left == fn1.args[0]
    assert fn1.body[0].value.right == fn1.args[1]

    s = ast_to_src(asr_to_ast(asrepr))
    assert s == """\
integer function fn1(a, b) result(r)
integer, intent(in) :: a
integer, intent(in) :: b
r = a + b
end function
"""

def test_statements():
    source = """\
integer :: a, b, r
r = a + b
"""
    ast = src_to_ast(source, translation_unit=False)
    asrepr = ast_to_asr(ast)
    verify_asr(asrepr)
    assert isinstance(asrepr, asr.TranslationUnit)
    assert 'a' in asrepr.global_scope.symbols
    assert 'b' in asrepr.global_scope.symbols
    assert 'r' in asrepr.global_scope.symbols
    body = asrepr.items
    assert isinstance(body[0], asr.Assignment)
    assert body[0].target == asrepr.global_scope.symbols["r"]
    assert body[0].value.left == asrepr.global_scope.symbols["a"]
    assert body[0].value.right == asrepr.global_scope.symbols["b"]

    s = ast_to_src(asr_to_ast(asrepr))
    assert s == """\
integer :: a
integer :: b
integer :: r
r = a + b
"""
