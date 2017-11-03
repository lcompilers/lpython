from liblfort.fortran_parser import parse


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
    assert parse("""\
subroutine a
x = "aaa" // str(x) // "bb"
y = "a" // "b"
end subroutine
""", r)

def test_arrays():
    r = "subroutine"
    assert parse("""\
subroutine f(e)
integer :: a(10,10), b(10)
integer, dimension(10,10) :: c
integer, dimension(:,:), intent(in) :: e
call g(a(3:5,i:j), b(:))
call g(a(:5,i:j), b(1:))
c = [1, 2, 3, i]
end subroutine
""", r)

def test_derived_type():
    r = "subroutine"
    assert parse("""\
subroutine f()
type(xx), intent(inout) :: x, y
real(dp) :: a, b
a = x%a
b = x%b(i, j)
c = y%c(5, :)
end subroutine
""", r)
    assert parse("""\
subroutine f()
type(xx), intent(inout) :: x, y
real(dp) :: a, b
a = x%f%a
b = x%g%b(i, j)
c = y%h%c(5, :)
end subroutine
""", r)
    assert parse("""\
subroutine f()
type(xx), intent(inout) :: x, y
real(dp) :: a, b
x%f%a = a
x%g%b(i, j) = b
y%h%c(5, :) = c
call x%f%e()
end subroutine
""", r)

def test_use_statement():
    r = "subroutine"
    assert parse("""\
subroutine f(e)
use a, b, c
use a, only: b, c
use a, only: x => b, c, d => a
integer :: a(10,10), b(10)
end subroutine
""", r)

def test_function():
    r = "function"
    assert parse("""\
real(dp) pure function f(e) result(r)
r = 1
end function
""", r)
    assert parse("""\
real(dp) recursive function f(e) result(r)
r = 1
end function
""", r)
    assert parse("""\
function f(e)
f = 1
end function
""", r)

def test_decl():
    r = "subroutine"
    assert parse("""\
subroutine f(e)
character(len=*) :: c
real(dp) y = 5
end subroutine
""", r)

def test_select():
    r = "subroutine"
    assert parse("""\
subroutine f(e)
select case(k)
    case(1)
        call a()
    case(i)
        call b()
end select
select case(k)
    case(1)
        call a()
    case(i)
        call b()
    case default
        call c()
end select
end subroutine
""", r)
