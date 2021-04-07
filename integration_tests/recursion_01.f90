module recursion_01
integer:: n
integer:: x = 0
contains

recursive subroutine sub1(x)
integer,intent(inout):: x
if (x < n) then
    x = x + 1
    print *, 'x = ', x
    call sub2()
    call sub1(x)
end if
    contains
    subroutine sub2()
        x = x + 1
        print *, x
        call sub1(x)
    end subroutine sub2
end subroutine sub1

end module recursion_01

program main
use recursion_01
n = 10
call sub1(x)
end program main
