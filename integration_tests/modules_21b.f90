module modules_21b

contains

subroutine sub(n)
integer, intent(in) :: n
real, allocatable, dimension(:) :: X
allocate(X(n))
end subroutine

end module
