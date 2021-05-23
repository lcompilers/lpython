module many_returns
implicit none

contains

integer function b(a)
integer :: a, e
print *, "calling b"
b = d(a)
contains
    integer function d(i)
        integer :: i
        if (i .EQ. 1) then
            d = 1
            return
        end if
        if (i .EQ. 2) then
            d = 2
            return
        end if
        if (i .EQ. 3) then
            d = 3
            return
        end if
        d = 999
    end function d
end function b

end module

program returns
use many_returns
implicit none
integer c
c = b(1)
print *, c
c = b(2)
print *, c
c = b(3)
print *, c
c = b(4)
print *, c
end program 
