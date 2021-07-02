program arrays_10
  implicit none
  integer, pointer :: x(:)
  allocate(x(1))
  x = [ 1 ]
end program arrays_10
