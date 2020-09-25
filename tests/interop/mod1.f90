module mod1
implicit none
private
public f1, f2, f3, f4, f2b, f3b, f5b

contains

integer function f1(a, b) result(r)
integer, intent(in) :: a, b
r = a + b
end function

integer function f2(n, a) result(r)
integer, intent(in) :: n, a(n)
r = sum(a)
end function

integer function f3(n, m, a) result(r)
integer, intent(in) :: n, m, a(n,m)
r = sum(a)
end function

integer function f2b(a) result(r)
integer, intent(in) :: a(:)
r = sum(a)
end function

integer function f3b(a) result(r)
integer, intent(in) :: a(:,:)
r = sum(a)
end function

integer function f4(a, b)
integer, intent(in) :: a
integer, intent(out) :: b
b = a
f4 = 0
end function

integer function f5b(a) result(r)
integer, intent(in) :: a(:,:)
integer :: i
r = 0
do i = 1, size(a, 2)
    r = r + a(1, i)
end do
end function

end module
