import pytest

from liblfort.semantic.analyze import (SymbolTableVisitor, Integer, Real,
        UndeclaredVariableError)
from liblfort.ast import parse

def test_variables():
    source = """\
module test
implicit none
contains

    subroutine sub1(a, b)
    integer, intent(in) :: a
    real, intent(out) :: b
    b = a + 1
    end subroutine

end module
"""
    tree = parse(source)
    v = SymbolTableVisitor()
    v.visit(tree)
    assert "a" in v.symbol_table
    assert isinstance(v.symbol_table["a"], Integer)
    assert v.symbol_table["a"].varname == "a"
    assert "b" in v.symbol_table
    assert isinstance(v.symbol_table["b"], Real)
    assert v.symbol_table["b"].varname == "b"
    assert not "c" in v.symbol_table
