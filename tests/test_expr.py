from fortran_parser import parse

def test_var():
    r = "var_sym_decl"
    assert parse("x", r)
    assert parse("yx", r)
    assert not parse("1x", r)
    assert not parse("2", r)

def test_expr1():
    r = "expr"
    assert parse("1", r)
    assert parse("x+y", r)
    assert parse("(1+3)*4", r)
    assert parse("1+3*4", r)
    assert not parse("(1+", r)

def test_expr2():
    r = "subroutine"
    assert parse("""\
subroutine a
integer :: a, b
a = 1+2*3
b = (1+2+a)*3
end subroutine
""", r)
    assert not parse("""\
subroutine a
integer :: a, b
a = 1+2*3
b = (1x+2+a)*3
end subroutine
""", r)
    assert parse("""\
subroutine a
integer :: b
b = 1+2*3
end subroutine
""", r)
    assert not parse("""\
subroutine a
integer :: b
b = 1+2*
end subroutine
""", r)
    assert not parse("""\
subroutine a
integer :: b
b = 1+
end subroutine
""", r)
    assert parse("""\
subroutine a
integer :: b
b = f(3)+6
end subroutine
""", r)
    assert not parse("""\
subroutine a
integer :: b
b = f(3+6
end subroutine
""", r)
    assert parse("""\
subroutine a
integer :: b
b = f(3+6)
end subroutine
""", r)

def test_statements():
    r = "subroutine"
    assert parse("""\
subroutine a
call random_number(u)
u = 2*u-1
r2 = sum(u**2)
u = u * sqrt(-2*log(r2)/r2)
x = u(1)
x = u(2)
first = .not. first
end subroutine
""", r)
