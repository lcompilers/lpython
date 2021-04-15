program parameter_04
implicit none
integer, parameter :: l_knd = kind(.true.)
integer, parameter :: sp = selected_real_kind(6)
integer, parameter :: dp = selected_real_kind(14)
integer, parameter :: i32 = selected_int_kind(6)
integer, parameter :: i64 = selected_int_kind(12)
logical(l_knd) :: l
real(sp) :: r1
real(dp) :: r2
integer(i32) :: i1
integer(i64) :: i2
l = .true.
print *, l_knd, sp, dp, i32, i64
end program
