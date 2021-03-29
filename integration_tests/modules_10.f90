module modules_10_b
implicit none
private
public b
contains
    integer function b()
    b = 5
    end function
end module

module modules_10_a
use modules_10_b, only: b
implicit none
private
public a
contains
    integer function a()
    a = 3 + b()
    end function
end module

program modules_10
use modules_10_a, only: a
implicit none
if (a() /= 8) error stop
print *, "OK"
end
