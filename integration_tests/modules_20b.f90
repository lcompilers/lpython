module modules_20b
implicit none

contains

subroutine trim2(y)
character(len=*),intent(in) :: y
print *, trim(y)
end subroutine

end module
