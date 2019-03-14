import pytest

from lfortran.semantic.ast_to_asr import ast_to_asr
from lfortran.ast import parse
from lfortran.asr import asr
from lfortran.asr.asr_check import verify_asr

from lfortran.ast.fortran_printer import print_fortran
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
    ast = parse(source)
    asrepr = ast_to_asr(ast)
    verify_asr(asrepr)
    assert 'modx' in asrepr.global_scope.symbols
    m = asrepr.global_scope.symbols['modx']
    assert isinstance(m, asr.Module)
    fn1 = m.symtab.symbols["fn1"]
    assert isinstance(fn1, asr.Function)
    assert fn1.args[0].name == "a"
    assert fn1.args[1].name == "b"
    assert fn1.return_var.name == "r"
    print(print_fortran(asr_to_ast(m)))