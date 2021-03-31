program modules_09
use modules_09_a, only: a
implicit none

if (a() /= 8) error stop

print *, "OK"

end
