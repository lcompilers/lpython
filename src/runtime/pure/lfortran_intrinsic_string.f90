module lfortran_intrinsic_string
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
! This does not work yet in LFortran:
!r = x(1:len(r))
! So we do this workaroud that works:
integer :: i
do i = 1, len(r)
    r(i:i) = x(i:i)
end do
end function

end module
