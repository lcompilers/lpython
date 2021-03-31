module modules_06_b
implicit none
private
public b

contains

integer function b()
b = 5
end function

end module

module modules_06_a
use modules_06_b, only: b
implicit none
private
public a

contains

integer function a()
a = 3 + b()
end function

end module
