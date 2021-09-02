module modules_20b
implicit none

contains

subroutine trim2(x)
character(len=*),intent(in) :: x
print *, trim(x)
end subroutine

end module
