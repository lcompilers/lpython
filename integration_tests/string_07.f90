program string_07

print *, f1(3)
if (len(f1(3)) /= 3) error stop
print *, f1(7)
if (len(f1(7)) /= 7) error stop

contains

function f1(n) result(r)
integer, intent(in) :: n
character(len=n) :: r
integer :: i
do i = 1, n
    r(i:i) = "a"
end do
end function

end program
