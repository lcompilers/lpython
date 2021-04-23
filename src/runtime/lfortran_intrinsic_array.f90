module lfortran_intrinsic_array
implicit none

contains

integer function size(x) result(r)
real(8), intent(in) :: x(:)
! TODO: implement size()
r = 0
end function

end module
