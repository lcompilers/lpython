module modules_09_a
use modules_09_b, only: b, i
implicit none
private
public a

contains

integer function a()
a = 3 + b() + i
end function

end module
