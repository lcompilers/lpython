program constant_kinds
implicit none

integer, parameter :: i1 = 1
integer, parameter :: i2 = 1_4
integer, parameter :: i3 = 1_8
real, parameter :: r1 = 1.0
real, parameter :: r2 = 1.d0
real, parameter :: r3 = 1.0_4
real, parameter :: r4 = 1.0_8

integer :: x
real :: y

y = 1.0
y = 1.d0
y = 1.0_4
y = 1.0_8
x = 1
x = 1_4
x = 1_8

end program
