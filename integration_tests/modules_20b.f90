module modules_20b
implicit none

contains

subroutine trim2(x)
character(len=*),intent(in) :: x
integer :: len_trim
len_trim = 1
print *, len_trim, trim(x)
end subroutine

end module
