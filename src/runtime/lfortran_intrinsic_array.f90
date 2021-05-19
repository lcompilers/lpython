module lfortran_intrinsic_array
implicit none

contains

integer function size(x) result(r)
! Type of x isn't needed but kept as integer here for syntactical correctness.
integer, intent(in) :: x(:)
end function

end module
