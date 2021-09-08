program string_06
character(*), parameter :: s1 = " A B ", s2 = " "

if (len_trim(s1) /= 4) error stop
if (len_trim(s2) /= 0) error stop
if (len_trim("  ") /= 0) error stop
if (len_trim("") /= 0) error stop
if (len_trim("xx") /= 2) error stop

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

end program
