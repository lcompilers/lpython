module recursion_02
implicit none

contains


integer function solver(f, iter)
interface
    integer function f()
    end function
end interface
!procedure(f_type) :: f
integer, intent(in) :: iter
print *, "before:", f()
solver = sub1(2, iter-1)
!solver = sub1(2, iter-1) + f()
print *, "after:", f()
end function


integer function sub1(y, iter)
integer, intent(in):: y, iter
integer x
integer tmp
x = y
print *, "in sub1"
if (iter == 1) then
    sub1 = 1
    return
end if
tmp = getx()
sub1 = solver(getx, iter)
contains

    integer function getx()
    print *, "x in getx", x
    getx = x
    end function

end function

end module recursion_02

program main
use recursion_02
implicit none
integer :: r
r = sub1(3, 3)
print *, "r =", r
end program main
