program test_mod1
use mod1, only: f1, f2, f3, f4, f2b, f3b
implicit none
integer :: x
call assert(f1(1, 2) == 3)
call assert(f2(3, [1, 2, 3]) == 6)
call assert(f2b([1, 2, 3]) == 6)
call assert(f3(2, 3, reshape([1, 2, 3, 4, 5, 6], [2,3])) == 21)
call assert(f3b(reshape([1, 2, 3, 4, 5, 6], [2,3])) == 21)

x = 0
call assert(f4(1, x) == 0)
call assert(x == 1)

contains

    subroutine assert(condition)
    logical, intent(in) :: condition
    if (.not. condition) error stop "Assert failed."
    end subroutine

end program
