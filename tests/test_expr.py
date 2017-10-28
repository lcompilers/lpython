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
    assert parse("""\
subroutine a
d = a - 1._dp/3
c = 1/sqrt(9*d)
v = (1 + c*x)**3
fn_val = d*v
exit
end subroutine
""", r)
    assert parse("""\
subroutine a
call randn(x(i))
call randn(x)
call random_number(U)
call rand_gamma0(a, .true., x)
call rand_gamma0(a, .true., x(1))
call rand_gamma0(a, .false., x(i))
call rand_gamma_vector_n(a, size(x), x)
end subroutine
""", r)
    assert parse("""\
subroutine a
x = 1; y = 2
y = 5; a = 1; x = u(2)
a = 5
end subroutine
""", r)
    assert parse("""\
subroutine a
x = 1; y = 2;
y = 5; a = 1; x = u(2);
a = 5;
; ;
end subroutine
""", r)

def test_control_flow():
    r = "subroutine"
    assert parse("""\
subroutine a
if (a) then
    x = 1
else
    x = 2
end if
end subroutine
""", r)
    assert parse("""\
subroutine a
if (a) then
    x = 1
else if (b) then
    x = 2
end if
end subroutine
""", r)
    assert parse("""\
subroutine a
if (a) then
    x = 1
else if (b) then
    x = 2
else if (c) then
    x = 2
end if
end subroutine
""", r)
    assert parse("""\
subroutine a
if (a) then
    x = 1
end if
end subroutine
""", r)
    assert parse("""\
subroutine a
if (a) x = 1
end subroutine
""", r)
    assert parse("""\
subroutine a
if (a) &
    x = 1
end subroutine
""", r)
    assert parse("""\
subroutine a
if (a) &     ! if statement
    x = 1
end subroutine
""", r)

    assert parse("""\
subroutine a
do
    x = 1
end do
end subroutine
""", r)
    assert parse("""\
subroutine a
do i = 1, 5
    x = x + i
end do
end subroutine
""", r)

def test_strings():
    r = "subroutine"
    assert parse("""\
subroutine a
x = "a'b'c"
y = 'a"b"c'
z = 'a""bc""x'
end subroutine
""", r)
    assert parse("""\
subroutine a
x = "a""c"
y = "a""b""c"
y = \"\"\"zippo\"\"\"
end subroutine
""", r)
    assert parse("""\
subroutine a
x = 'a''c'
y = 'a''b''c'
y = '''zippo'''
end subroutine
""", r)

def test_arrays():
    r = "subroutine"
    assert parse("""\
subroutine f()
integer :: a(10,10), b(10)
call g(a(3:5,i:j), b(:))
call g(a(:5,i:j), b(1:))
end subroutine
""", r)
