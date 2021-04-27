module lfortran_intrinsic_array
implicit none

contains

integer function size(x) result(r)
real(8), intent(in) :: x(:)
end function

integer function lbound(array, dim, kind) result(lbounds)
real, intent(in) :: array(:)
integer, optional, intent(in) :: dim, kind
end function

end module
