program types_06
implicit none
real :: r
integer :: i
r = 2
i = 2

if (i < i) error stop
if (r < r) error stop
if (r < i) error stop
if (i < r) error stop

if (i > i) error stop
if (r > r) error stop
if (r > i) error stop
if (i > r) error stop

if (i /= i) error stop
if (r /= r) error stop
if (r /= i) error stop
if (i /= r) error stop

if (i+1 <= i) error stop
if (r+1 <= r) error stop
if (r+1 <= i) error stop
if (i+1 <= r) error stop

if (i >= i+1) error stop
if (r >= r+1) error stop
if (r >= i+1) error stop
if (i >= r+1) error stop

if (i == i+1) error stop
if (r == r+1) error stop
if (r == i+1) error stop
if (i == r+1) error stop

end program
