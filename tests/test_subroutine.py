from fortran_parser import parse

def test_subroutine():
    r = "subroutine"
    assert parse("""\
subroutine f
integer :: x
end subroutine
""", r)
    assert parse("""\
subroutine f()
! Some comment
integer :: x
! Some other comment
end subroutine
""", r)
    assert parse("""\
subroutine f(a, b, c, d)
integer, intent(in) :: a, b
integer, intent ( in ) :: c, d
integer :: z
integer::y
end subroutine
""", r)
    assert parse("""\
subroutine f(a, b, c, d)
integer, intent(out) :: a, b
integer, intent(inout) :: c, d
integer :: z
integer::y
end subroutine
""", r)
