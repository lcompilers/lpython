module string_08_mod
implicit none

contains

integer elemental function len_trim(string) result(r)
character(len=*), intent(in) :: string
r = len(string)
if (r == 0) return
do while(string(r:r) == " ")
    r = r - 1
    if (r == 0) exit
end do
end function

function trim(x) result(r)
character(len=*),intent(in) :: x
character(len=len_trim(x)) :: r
integer :: i
do i = 1, len(r)
    r(i:i) = x(i:i)
end do
end function

end module

program string_08
use string_08_mod
implicit none
character(*), parameter :: s1 = " A B ", s2 = " "

if (len_trim(s1) /= 4) error stop
if (len_trim(s2) /= 0) error stop
if (len_trim("  ") /= 0) error stop
if (len_trim("") /= 0) error stop
if (len_trim("xx") /= 2) error stop

if (trim(s1) /= " A B") error stop
if (trim(s2) /= "") error stop
if (trim("  ") /= "") error stop
if (trim("") /= "") error stop
if (trim("xx") /= "xx") error stop

if (len(trim(s1)) /= 4) error stop
if (len(trim(s2)) /= 0) error stop
if (len(trim("  ")) /= 0) error stop
if (len(trim("")) /= 0) error stop
if (len(trim("xx")) /= 2) error stop

print *, trim("xx    ")
print *, len(trim("xx    "))

end program
