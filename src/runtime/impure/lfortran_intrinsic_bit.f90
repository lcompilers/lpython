module lfortran_intrinsic_bit
use, intrinsic :: iso_fortran_env, only: int32, int64
implicit none

interface iand
    module procedure iand32, iand64
end interface

interface not
    module procedure not32, not64
end interface

interface ior
    module procedure ior32, ior64
end interface

interface ieor
    module procedure ieor32, ieor64
end interface

interface ibclr
    module procedure ibclr32, ibclr64
end interface

interface ibset
    module procedure ibset32, ibset64
end interface

interface btest
    module procedure btest32, btest64
end interface

interface ishft
    module procedure ishft32, ishft64
end interface

interface mvbits
    module procedure mvbits32, mvbits64
end interface

interface bge
    module procedure bge32, bge64
end interface

interface bgt
    module procedure bgt32, bgt64
end interface

interface ble
    module procedure ble32, ble64
end interface

interface blt
    module procedure blt32, blt64
end interface

interface ibits
    module procedure ibits32, ibits64
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

! not --------------------------------------------------------------------------

elemental integer(int32) function not32(x) result(r)
integer(int32), intent(in) :: x
interface
    pure integer(int32) function c_not32(x) bind(c, name="_lfortran_not32")
    import :: int32
    integer(int32), intent(in), value :: x
    end function
end interface
r = c_not32(x)
end function

elemental integer(int64) function not64(x) result(r)
integer(int64), intent(in) :: x
interface
    pure integer(int64) function c_not64(x) bind(c, name="_lfortran_not64")
    import :: int64
    integer(int64), intent(in), value :: x
    end function
end interface
r = c_not64(x)
end function

! ior --------------------------------------------------------------------------

elemental integer(int32) function ior32(x, y) result(r)
integer(int32), intent(in) :: x
integer(int32), intent(in) :: y
interface
    pure integer(int32) function c_ior32(x, y) bind(c, name="_lfortran_ior32")
    import :: int32
    integer(int32), intent(in), value :: x
    integer(int32), intent(in), value :: y
    end function
end interface
r = c_ior32(x, y)
end function

elemental integer(int64) function ior64(x, y) result(r)
integer(int64), intent(in) :: x
integer(int64), intent(in) :: y
interface
    pure integer(int64) function c_ior64(x, y) bind(c, name="_lfortran_ior64")
    import :: int64
    integer(int64), intent(in), value :: x
    integer(int64), intent(in), value :: y
    end function
end interface
r = c_ior64(x, y)
end function

! ieor --------------------------------------------------------------------------

elemental integer(int32) function ieor32(x, y) result(r)
integer(int32), intent(in) :: x
integer(int32), intent(in) :: y
interface
    pure integer(int32) function c_ieor32(x, y) bind(c, name="_lfortran_ieor32")
    import :: int32
    integer(int32), intent(in), value :: x
    integer(int32), intent(in), value :: y
    end function
end interface
r = c_ieor32(x, y)
end function

elemental integer(int64) function ieor64(x, y) result(r)
integer(int64), intent(in) :: x
integer(int64), intent(in) :: y
interface
    pure integer(int64) function c_ieor64(x, y) bind(c, name="_lfortran_ieor64")
    import :: int64
    integer(int64), intent(in), value :: x
    integer(int64), intent(in), value :: y
    end function
end interface
r = c_ieor64(x, y)
end function

! ibclr --------------------------------------------------------------------------

elemental integer(int32) function ibclr32(i, pos) result(r)
integer(int32), intent(in) :: i
integer, intent(in) :: pos
interface
    pure integer(int32) function c_ibclr32(i, pos) bind(c, name="_lfortran_ibclr32")
    import :: int32
    integer(int32), intent(in), value :: i
    integer, intent(in), value :: pos
    end function
end interface

if (pos >= 0 .and. pos < 32) then
    r = c_ibclr32(i, pos)
else
    error stop "ibclr(i, pos) for pos < 0 or pos >= bit_size(i) is not allowed"
end if
end function

elemental integer(int64) function ibclr64(i, pos) result(r)
integer(int64), intent(in) :: i
integer, intent(in) :: pos
interface
    pure integer(int64) function c_ibclr64(i, pos) bind(c, name="_lfortran_ibclr64")
    import :: int64
    integer(int64), intent(in), value :: i
    integer, intent(in), value :: pos
    end function
end interface

if (pos >= 0 .and. pos < 64) then
    r = c_ibclr64(i, pos)
else
    error stop "ibclr(i, pos) for pos < 0 or pos >= bit_size(i) is not allowed"
end if
end function

! ibset --------------------------------------------------------------------------

elemental integer(int32) function ibset32(i, pos) result(r)
integer(int32), intent(in) :: i
integer, intent(in) :: pos
interface
    pure integer(int32) function c_ibset32(i, pos) bind(c, name="_lfortran_ibset32")
    import :: int32
    integer(int32), intent(in), value :: i
    integer, intent(in), value :: pos
    end function
end interface

if (pos >= 0 .and. pos < 32) then
    r = c_ibset32(i, pos)
else
    error stop "ibset(i, pos) for pos < 0 or pos >= bit_size(i) is not allowed"
end if
end function

elemental integer(int64) function ibset64(i, pos) result(r)
integer(int64), intent(in) :: i
integer, intent(in) :: pos
interface
    pure integer(int64) function c_ibset64(i, pos) bind(c, name="_lfortran_ibset64")
    import :: int64
    integer(int64), intent(in), value :: i
    integer, intent(in), value :: pos
    end function
end interface

if (pos >= 0 .and. pos < 64) then
    r = c_ibset64(i, pos)
else
    error stop "ibset(i, pos) for pos < 0 or pos >= bit_size(i) is not allowed"
end if
end function

! btest --------------------------------------------------------------------------

elemental logical function btest32(i, pos) result(r)
integer(int32), intent(in) :: i
integer, intent(in) :: pos
interface
    pure integer(int32) function c_btest32(i, pos) bind(c, name="_lfortran_btest32")
    import :: int32
    integer(int32), intent(in), value :: i
    integer, intent(in), value :: pos
    end function
end interface

if (pos >= 0 .and. pos < 32) then
    r = c_btest32(i, pos) /= 0
else
    error stop "btest(i, pos) for pos < 0 or pos >= bit_size(i) is not allowed"
end if
end function

elemental logical function btest64(i, pos) result(r)
integer(int64), intent(in) :: i
integer, intent(in) :: pos
interface
    pure integer(int64) function c_btest64(i, pos) bind(c, name="_lfortran_btest64")
    import :: int64
    integer(int64), intent(in), value :: i
    integer, intent(in), value :: pos
    end function
end interface

if (pos >= 0 .and. pos < 64) then
    r = c_btest64(i, pos) /= 0
else
    error stop "btest(i, pos) for pos < 0 or pos >= bit_size(i) is not allowed"
end if
end function

! ishft ------------------------------------------------------------------------

elemental integer(int32) function ishft32(i, shift) result(r)
integer(int32), intent(in) :: i, shift
interface
    pure integer(int32) function c_ishft32(i, shift) bind(c, name="_lfortran_ishft32")
        import :: int32
        integer(int32), intent(in), value :: i, shift
    end function
end interface

if (shift < 32) then
    r = c_ishft32(i, shift)
else
    error stop "shift must be less than 32"
end if
end function

elemental integer(int64) function ishft64(i, shift) result(r)
integer(int64), intent(in) :: i, shift
interface
    pure integer(int64) function c_ishft64(i, shift) bind(c, name="_lfortran_ishft64")
        import :: int64
        integer(int64), intent(in), value :: i, shift
    end function
end interface

if (shift < 64) then
    r = c_ishft64(i, shift)
else
    error stop "shift must be less than 64"
end if
end function

! mvbits ------------------------------------------------------------------------

elemental subroutine mvbits32(from, frompos, len, to, topos)
integer(int32), intent(in) :: from, frompos, len, topos
integer(int32), intent(out) :: to
interface
    pure integer(int32) function c_mvbits32(from, frompos, len, to, topos) bind(c, name="_lfortran_mvbits32")
    import :: int32
    integer(int32), intent(in), value :: from, frompos, len, to, topos
    end function
end interface
to = c_mvbits32(from, frompos, len, to, topos)
end subroutine

elemental subroutine mvbits64(from, frompos, len, to, topos)
integer(int64), intent(in) :: from
integer(int32), intent(in) :: frompos, len, topos
integer(int64), intent(out) :: to
interface
    pure integer(int64) function c_mvbits64(from, frompos, len, to, topos) bind(c, name="_lfortran_mvbits64")
    import :: int64, int32
    integer(int64), intent(in), value :: from, to
    integer(int32), intent(in), value :: frompos, len, topos
    end function
end interface
to = c_mvbits64(from, frompos, len, to, topos)
end subroutine

! bge ------------------------------------------------------------------------

elemental logical function bge32(i, j) result(r)
integer(int32), intent(in) :: i, j
interface
    pure logical function c_bge32(i, j) bind(c, name="_lfortran_bge32")
    import :: int32
    integer(int32), intent(in), value :: i, j
    end function
end interface
r = c_bge32(i, j)
end function

elemental logical function bge64(i, j) result(r)
integer(int64), intent(in) :: i, j
interface
    pure logical function c_bge64(i, j) bind(c, name="_lfortran_bge64")
    import :: int64
    integer(int64), intent(in), value :: i, j
    end function
end interface
r = c_bge64(i, j)
end function

! bgt ------------------------------------------------------------------------

elemental logical function bgt32(i, j) result(r)
integer(int32), intent(in) :: i, j
interface
    pure logical function c_bgt32(i, j) bind(c, name="_lfortran_bgt32")
    import :: int32
    integer(int32), intent(in), value :: i, j
    end function
end interface
r = c_bgt32(i, j)
end function

elemental logical function bgt64(i, j) result(r)
integer(int64), intent(in) :: i, j
interface
    pure logical function c_bgt64(i, j) bind(c, name="_lfortran_bgt64")
    import :: int64
    integer(int64), intent(in), value :: i, j
    end function
end interface
r = c_bgt64(i, j)
end function

! ble ------------------------------------------------------------------------

elemental logical function ble32(i, j) result(r)
integer(int32), intent(in) :: i, j
interface
    pure logical function c_ble32(i, j) bind(c, name="_lfortran_ble32")
    import :: int32
    integer(int32), intent(in), value :: i, j
    end function
end interface
r = c_ble32(i, j)
end function

elemental logical function ble64(i, j) result(r)
integer(int64), intent(in) :: i, j
interface
    pure logical function c_ble64(i, j) bind(c, name="_lfortran_ble64")
    import :: int64
    integer(int64), intent(in), value :: i, j
    end function
end interface
r = c_ble64(i, j)
end function

! blt ------------------------------------------------------------------------

elemental logical function blt32(i, j) result(r)
integer(int32), intent(in) :: i, j
interface
    pure logical function c_blt32(i, j) bind(c, name="_lfortran_blt32")
    import :: int32
    integer(int32), intent(in), value :: i, j
    end function
end interface
r = c_blt32(i, j)
end function

elemental logical function blt64(i, j) result(r)
integer(int64), intent(in) :: i, j
interface
    pure logical function c_blt64(i, j) bind(c, name="_lfortran_blt64")
    import :: int64
    integer(int64), intent(in), value :: i, j
    end function
end interface
r = c_blt64(i, j)
end function

! ibits ------------------------------------------------------------------------

elemental integer(int32) function ibits32(i, pos, len) result(r)
integer(int32), intent(in) :: i, pos, len
interface
    pure integer(int32) function c_ibits32(i, pos, len) bind(c, name="_lfortran_ibits32")
    import :: int32
    integer(int32), intent(in), value :: i, pos, len
    end function
end interface
r = c_ibits32(i, pos, len)
end function

elemental integer(int64) function ibits64(i, pos, len) result(r)
integer(int64), intent(in) :: i
integer(int32), intent(in) :: pos, len
interface
    pure integer(int64) function c_ibits64(i, pos, len) bind(c, name="_lfortran_ibits64")
    import :: int32, int64
    integer(int64), intent(in), value :: i
    integer(int32), intent(in), value :: pos, len
    end function
end interface
r = c_ibits64(i, pos, len)
end function

end module
