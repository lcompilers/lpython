program bugSize
  integer, pointer :: x(:)
  allocate(x(4))
  call f(x)

contains

subroutine f(x)
integer, intent(in) :: x(:)
real :: E(size(x))
E = x
E = [ 1, 2, 3, 4 ]
print*, E
end subroutine

end program
