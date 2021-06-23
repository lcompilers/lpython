program intrinsics_07
! Tests for kind()
integer, parameter :: sp = kind(0.0)
integer, parameter :: sp2 = kind(0.e0)
integer, parameter :: dp = kind(0.d0)
real(sp) :: a
real(dp) :: b
a = 5
b = a
end
