program modules_08
use modules_08_a, only: a
use modules_08_b, only: b
implicit none

if (a() /= 3) error stop
if (b() /= 5) error stop

print *, "OK"

end
