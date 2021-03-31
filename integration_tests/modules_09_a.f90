module modules_09_a
use modules_09_b, only: b
implicit none
private
public a

contains

integer function a()
a = 3 + b()
end function

end module
