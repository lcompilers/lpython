program project1
use a, only: f, fib
implicit none

if (f() /= 8) error stop

call fib(10)

print *, "OK"

end
