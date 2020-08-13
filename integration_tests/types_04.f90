program types_04
implicit none
real :: r, x
integer :: i
r = 1.5
i = 2

x = i*i
x = r*r
x = i*r
x = r*i

x = i+i
x = r+r
x = r+i
x = i+r

x = i-i
x = r-r
x = r-i
x = i-r

x = i/i
x = r/r
x = i/r
x = r/i
end program
