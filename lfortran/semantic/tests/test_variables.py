import pytest

from lfortran.semantic.analyze import (create_symbol_table, Integer, Real,
        Array, Logical, UndeclaredVariableError, annotate_tree, TypeMismatch)
from lfortran.ast import src_to_ast, dump

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

def test_types_arrays():
    i = Integer()
    dims = [9, 10]
    a = Array(i, dims)
    assert a == a
    assert not (a != a)
    assert a == Array(i, dims)
    assert a == Array(Integer(), [9, 10])
    assert a != Array(Integer(), [9, 11])
    assert a != Array(Real(), [9, 10])

def test_variables1():
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
    tree = src_to_ast(source)
    global_scope = create_symbol_table(tree)
    assert not global_scope.resolve("a", False)
    sym = tree.contains[0]._scope.resolve("a")
    assert sym["type"] == Integer()
    assert sym["name"] == "a"
    sym = tree.contains[0]._scope.resolve("b")
    assert sym["type"] == Real()
    assert sym["name"] == "b"
    assert not tree.contains[0]._scope.resolve("c", False)

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
    tree = src_to_ast(source)
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
    tree = src_to_ast(source)
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
    tree = src_to_ast(source, False)
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
    tree = src_to_ast(source)
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
    tree = src_to_ast(source)
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
    tree = src_to_ast(source)
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
    tree = src_to_ast(source)
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
    tree = src_to_ast(source)
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
    tree = src_to_ast(source)
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
    tree = src_to_ast(source)
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
    tree = src_to_ast(source, False)
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
    tree = src_to_ast(source, False)
    symbol_table = create_symbol_table(tree)
    annotate_tree(tree, symbol_table)
    assert dump(tree, include_type=True) == "Subroutine(name='sub1', args=[arg(arg='a'), arg(arg='b')], use=[], decl=[Declaration(vars=[decl(sym='a', sym_type='integer', dims=[], attrs=[Attribute(name='intent', args=[attribute_arg(arg='in')])])]), Declaration(vars=[decl(sym='b', sym_type='integer', dims=[], attrs=[Attribute(name='intent', args=[attribute_arg(arg='in')])])])], body=[Assignment(target=Name(id='a'), value=BinOp(left=BinOp(left=BinOp(left=Name(id='b', type=Integer()), op=Add(), right=Num(n='1', type=Integer()), type=Integer()), op=Mul(), right=Name(id='a', type=Integer()), type=Integer()), op=Mul(), right=Num(n='5', type=Integer()), type=Integer()), type=Integer())], contains=[])"

def test_logical1():
    source = """\
subroutine sub1(a, b)
integer, intent(in) :: a
integer, intent(in) :: b
logical :: r
r = (a == b)
end subroutine
"""
    tree = src_to_ast(source, False)
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
    tree = src_to_ast(source, False)
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
    tree = src_to_ast(source, False)
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
    tree = src_to_ast(source, False)
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
    tree = src_to_ast(source, False)
    symbol_table = create_symbol_table(tree)
    with pytest.raises(UndeclaredVariableError):
        annotate_tree(tree, symbol_table)

def test_logical6():
    source = """\
subroutine sub1(a, b)
integer, intent(in) :: a(3), b(3)
integer :: c
if (a == b) c = 1
if (a(1) == 1) c = 1
if (1 == a(1)) c = 1
if (a(1) == a(1)) c = 1
end subroutine
"""
    tree = src_to_ast(source, False)
    symbol_table = create_symbol_table(tree)
    annotate_tree(tree, symbol_table)

def test_arrays1():
    source = """\
subroutine sub1()
integer :: a(3), i
a(1) = 1
i = 2
a(2) = i
a(i) = i+1
end subroutine
"""
    tree = src_to_ast(source, False)
    symbol_table = create_symbol_table(tree)
    annotate_tree(tree, symbol_table)

def test_arrays2():
    source = """\
subroutine sub1()
integer :: a(3)
real :: r
a(1) = 1
r = 2
a(r) = 2
end subroutine
"""
    tree = src_to_ast(source, False)
    symbol_table = create_symbol_table(tree)
    with pytest.raises(TypeMismatch):
        annotate_tree(tree, symbol_table)

def test_arrays3():
    # Here we test assigning the wrong type to an array. Since only real and
    # integer types are implemented for now, just test that real cannot be
    # assinged to an integer. Later we can change this to, say, a character.
    source = """\
subroutine sub1()
integer :: a(3), i
real :: r
i = 1
a(i) = r
end subroutine
"""
    tree = src_to_ast(source, False)
    symbol_table = create_symbol_table(tree)
    with pytest.raises(TypeMismatch):
        annotate_tree(tree, symbol_table)

def test_arrays4():
    source = """\
subroutine sub1()
integer :: a(3), b(4), i
a(1) = b(2)
a(i) = b(2)
a(1) = b(2) + 1
a(1) = 1 + b(2)
a(1) = b(i) + 1
a(1) = b(i) + b(1)
end subroutine
"""
    tree = src_to_ast(source, False)
    symbol_table = create_symbol_table(tree)
    annotate_tree(tree, symbol_table)


def test_subroutines1():
    source = """\
subroutine sub1(a, b)
integer, intent(in) :: a
integer, intent(out) :: b
b = a + 1
end subroutine
"""
    tree = src_to_ast(source, False)
    symbol_table = create_symbol_table(tree)
    annotate_tree(tree, symbol_table)
