module recursion_03
implicit none

contains


integer function solver_caller(f, iter)
interface
    integer function f()
    end function
end interface
integer, intent(in) :: iter
solver_caller = solver(f, iter)
end function

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
sub1 = solver_caller(getx, iter)
contains

    integer function getx()
    print *, "x in getx", x
    getx = x
    end function

end function

end module recursion_03

program main
use recursion_03
implicit none
integer :: r
r = sub1(3, 3)
print *, "r =", r
end program main
