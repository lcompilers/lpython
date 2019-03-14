import pytest

from lfortran.semantic.ast_to_asr import ast_to_asr
from lfortran.ast import parse
from lfortran.asr import asr

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
    assert 'modx' in asrepr.global_scope.symbols
    m = asrepr.global_scope.symbols['modx']
    assert isinstance(m, asr.Module)
    fn1 = m.symtab.symbols["fn1"]
    assert isinstance(fn1, asr.Function)
    assert fn1.args[0].name == "a"
    assert fn1.args[1].name == "b"