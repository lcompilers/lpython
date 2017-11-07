import pytest

from liblfort.semantic.analyze import (create_symbol_table, Integer, Real,
        Logical, UndeclaredVariableError, annotate_tree, TypeMismatch)
from liblfort.ast import parse, dump

def test_types():
    r = Real()
    i = Integer()
    assert r == r
    assert not (r != r)
    assert r == Real()

    assert i == i
    assert not (i != i)
    assert i == Integer()

    assert i != r
    assert not (i == r)
    assert not (i == Real())
    assert i != Real()

    assert r != i
    assert not (r == i)
    assert not (r == Integer())
    assert r != Integer()

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
    symbol_table = create_symbol_table(tree)
    assert "a" in symbol_table
    assert symbol_table["a"]["type"] == Integer()
    assert symbol_table["a"]["name"] == "a"
    assert "b" in symbol_table
    assert symbol_table["b"]["type"] == Real()
    assert symbol_table["b"]["name"] == "b"
    assert not "c" in symbol_table

def test_unknown_type1():
    source = """\
module test
implicit none
contains

    subroutine sub1(a)
    integer, intent(in) :: a
    a = b
    end subroutine

end module
"""
    tree = parse(source)
    symbol_table = create_symbol_table(tree)
    with pytest.raises(UndeclaredVariableError):
        annotate_tree(tree, symbol_table)

def test_unknown_type2():
    source = """\
module test
implicit none
contains

    subroutine sub1(a)
    integer, intent(in) :: a
    a = (1+b)**2
    end subroutine

end module
"""
    tree = parse(source)
    symbol_table = create_symbol_table(tree)
    with pytest.raises(UndeclaredVariableError):
        annotate_tree(tree, symbol_table)

def test_unknown_type2b():
    source = """\
subroutine sub1(a)
integer, intent(in) :: a
b = 1
end subroutine
"""
    tree = parse(source, False)
    symbol_table = create_symbol_table(tree)
    with pytest.raises(UndeclaredVariableError):
        annotate_tree(tree, symbol_table)

def test_unknown_type3():
    source = """\
module test
implicit none
contains

    subroutine sub1(a)
    integer, intent(in) :: a
    a = (1+a)**2
    end subroutine

end module
"""
    tree = parse(source)
    symbol_table = create_symbol_table(tree)
    annotate_tree(tree, symbol_table)

def test_type1():
    source = """\
module test
implicit none
contains

    subroutine sub1(a, b)
    integer, intent(in) :: a, b
    a = b
    end subroutine

end module
"""
    tree = parse(source)
    symbol_table = create_symbol_table(tree)
    annotate_tree(tree, symbol_table)
    assert tree.contains[0].body[0]._type == Integer()

def test_type2():
    source = """\
module test
implicit none
contains

    subroutine sub1(a, b)
    integer, intent(in) :: a
    real, intent(in) :: b
    a = b
    end subroutine

end module
"""
    tree = parse(source)
    symbol_table = create_symbol_table(tree)
    with pytest.raises(TypeMismatch):
        annotate_tree(tree, symbol_table)

def test_type3():
    source = """\
module test
implicit none
contains

    subroutine sub1(a, b)
    integer, intent(in) :: a
    integer, intent(in) :: b
    a = b + 1
    end subroutine

end module
"""
    tree = parse(source)
    symbol_table = create_symbol_table(tree)
    annotate_tree(tree, symbol_table)
    assert tree.contains[0].body[0]._type == Integer()

def test_type4():
    source = """\
module test
implicit none
contains

    subroutine sub1(a, b)
    integer, intent(in) :: a
    real, intent(in) :: b
    a = b + 1
    end subroutine

end module
"""
    tree = parse(source)
    symbol_table = create_symbol_table(tree)
    with pytest.raises(TypeMismatch):
        annotate_tree(tree, symbol_table)

def test_type5():
    source = """\
module test
implicit none
contains

    subroutine sub1(a, b)
    integer, intent(in) :: a
    integer, intent(in) :: b
    a = (b + 1)*a*5
    end subroutine

end module
"""
    tree = parse(source)
    symbol_table = create_symbol_table(tree)
    annotate_tree(tree, symbol_table)
    assert tree.contains[0].body[0]._type == Integer()

def test_type6():
    source = """\
module test
implicit none
contains

    subroutine sub1(a, b)
    integer, intent(in) :: a
    real, intent(in) :: b
    a = (b + 1)*a*5
    end subroutine

end module
"""
    tree = parse(source)
    symbol_table = create_symbol_table(tree)
    with pytest.raises(TypeMismatch):
        annotate_tree(tree, symbol_table)

def test_type7():
    source = """\
subroutine sub1(a, b)
real, intent(in) :: a
real, intent(in) :: b
a = (b + a)*a
end subroutine
"""
    tree = parse(source, False)
    symbol_table = create_symbol_table(tree)
    annotate_tree(tree, symbol_table)
    assert tree.body[0]._type == Real()

def test_dump():
    source = """\
subroutine sub1(a, b)
integer, intent(in) :: a
integer, intent(in) :: b
a = (b + 1)*a*5
end subroutine
"""
    tree = parse(source, False)
    symbol_table = create_symbol_table(tree)
    annotate_tree(tree, symbol_table)
    assert dump(tree, include_type=True) == "Subroutine(name='sub1', args=[arg(arg='a'), arg(arg='b')], decl=[Declaration(vars=[decl(sym='a', sym_type='integer')]), Declaration(vars=[decl(sym='b', sym_type='integer')])], body=[Assignment(target='a', value=BinOp(left=BinOp(left=BinOp(left=Name(id='b', type=Integer()), op=Add(), right=Num(n='1', type=Integer()), type=Integer()), op=Mul(), right=Name(id='a', type=Integer()), type=Integer()), op=Mul(), right=Num(n='5', type=Integer()), type=Integer()), type=Integer())], contains=[])"

def test_logical1():
    source = """\
subroutine sub1(a, b)
integer, intent(in) :: a
integer, intent(in) :: b
logical :: r
r = (a == b)
end subroutine
"""
    tree = parse(source, False)
    symbol_table = create_symbol_table(tree)
    annotate_tree(tree, symbol_table)
    assert tree.body[0]._type == Logical()

def test_logical2():
    source = """\
subroutine sub1(a)
integer, intent(in) :: a
logical :: r
r = a
end subroutine
"""
    tree = parse(source, False)
    symbol_table = create_symbol_table(tree)
    with pytest.raises(TypeMismatch):
        annotate_tree(tree, symbol_table)

def test_logical3():
    source = """\
subroutine sub1(a, b)
integer, intent(in) :: a, b
integer :: c
if (a == b) c = 1
end subroutine
"""
    tree = parse(source, False)
    symbol_table = create_symbol_table(tree)
    annotate_tree(tree, symbol_table)

def test_logical4():
    source = """\
subroutine sub1(a, b)
integer, intent(in) :: a, b
integer :: c
if (a) c = 1
end subroutine
"""
    tree = parse(source, False)
    symbol_table = create_symbol_table(tree)
    with pytest.raises(TypeMismatch):
        annotate_tree(tree, symbol_table)

def test_logical5():
    source = """\
subroutine sub1(a, b)
integer, intent(in) :: a, b
integer :: c
if (a == b) then
    d = 1
end if
end subroutine
"""
    tree = parse(source, False)
    symbol_table = create_symbol_table(tree)
    with pytest.raises(UndeclaredVariableError):
        annotate_tree(tree, symbol_table)
