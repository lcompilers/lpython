module lfortran_intrinsic_kind
implicit none

! Does not work yet:
!
!interface kind
!    module procedure skind, dkind, lkind
!end interface


contains

integer function kind(x) result(r)
logical(4), intent(in) :: x
r = 4
end function

integer function skind(x) result(r)
real(4), intent(in) :: x
r = 4
end function

integer function dkind(x) result(r)
real(8), intent(in) :: x
r = 8
end function

integer function lkind(x) result(r)
logical(4), intent(in) :: x
r = 4
end function

integer function selected_int_kind(x) result(r)
integer, intent(in) :: x
r = 4
end function

integer function selected_real_kind(x) result(r)
integer, intent(in) :: x
r = 4
end function

end module
