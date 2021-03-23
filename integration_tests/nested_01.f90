module a
implicit none

contains

integer function b()
integer e
print *, "b()"
e = d()
b = 0
contains
    integer function d()
        print *, "d()"
        d = 1
    end function d
end function b

end module

program nested_01
use a, only: b
implicit none
integer c
c = b()

end
