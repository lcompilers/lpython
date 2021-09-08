module string_07_mod
implicit none

contains

integer pure function some_fn(a) result(r)
integer, intent(in) :: a
r = a
end function

end module

program string_07
use string_07_mod
implicit none

print *, f1(3)
if (len(f1(3)) /= 3) error stop
print *, f1(7)
if (len(f1(7)) /= 7) error stop

print *, f2(3)
if (len(f2(3)) /= 3) error stop
print *, f2(7)
if (len(f2(7)) /= 7) error stop

contains

function f1(n) result(r)
integer, intent(in) :: n
character(len=n) :: r
integer :: i
do i = 1, n
    r(i:i) = "a"
end do
end function

function f2(n) result(r)
integer, intent(in) :: n
character(len=some_fn(n)) :: r
integer :: i
do i = 1, n
    r(i:i) = "a"
end do
end function

end program
