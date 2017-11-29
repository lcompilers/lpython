program intrinsics_01
integer, parameter :: dp = kind(0.d0)
real(dp) :: a, b
a = 1.1_dp
b = 1.2_dp
if (b-a > 0.2_dp) error stop
if (abs(b-a) > 0.2_dp) error stop
if (abs(a-b) > 0.2_dp) error stop

a = 4._dp
if (abs(sqrt(a)-2._dp) > 1e-12_dp) error stop
end
