from liblfort.semantic.analyze import SymbolTableVisitor
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
    assert v.symbol_table["a"]["type"] == "integer"
    assert "b" in v.symbol_table
    assert v.symbol_table["b"]["type"] == "real"
    assert not "c" in v.symbol_table
