program intrinsics_01
integer, parameter :: dp = kind(0.d0)
real(dp) :: a, b, c(4)
integer :: i
a = 1.1_dp
b = 1.2_dp
if (b-a > 0.2_dp) error stop
if (abs(b-a) > 0.2_dp) error stop
if (abs(a-b) > 0.2_dp) error stop

a = 4._dp
if (abs(sqrt(a)-2._dp) > 1e-12_dp) error stop

a = 4._dp
if (abs(log(a)-1.3862943611198906_dp) > 1e-12_dp) error stop

call random_number(c)
do i = 1, 4
    if (c(i) < 0._dp) error stop
    if (c(i) > 1._dp) error stop
end do
end
