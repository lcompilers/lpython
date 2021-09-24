module lfortran_intrinsic_bit
use, intrinsic :: iso_fortran_env, only: int32, int64
implicit none

interface iand
    module procedure iand32, iand64
end interface

contains

! iand --------------------------------------------------------------------------

elemental integer(int32) function iand32(x, y) result(r)
integer(int32), intent(in) :: x
integer(int32), intent(in) :: y
interface
    pure integer(int32) function c_iand32(x, y) bind(c, name="_lfortran_iand32")
    import :: int32
    integer(int32), intent(in), value :: x
    integer(int32), intent(in), value :: y
    end function
end interface
r = c_iand32(x, y)
end function

elemental integer(int64) function iand64(x, y) result(r)
integer(int64), intent(in) :: x
integer(int64), intent(in) :: y
interface
    pure integer(int64) function c_iand64(x, y) bind(c, name="_lfortran_iand64")
    import :: int64
    integer(int64), intent(in), value :: x
    integer(int64), intent(in), value :: y
    end function
end interface
r = c_iand64(x, y)
end function

end module
