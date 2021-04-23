program functions_03
implicit none
integer, parameter :: sp = 4
complex(4) :: i, j
i = 1
j = 1
print *, j
j = f(i)
print *, i
print *, j

j = 1
print *, j
j = f((3.0, 0.0))
print *, j

j = 1
print *, j
j = g((2.0, 0.0))
print *, j

j = 1
print *, j
j = g((2.0, 3.0))
print *, j

contains

    complex(4) function f(a) result(b)
    complex(4), intent(in) :: a
    b = a + 5
    end function

    complex(sp) function g(a) result(b)
    complex(sp), intent(in) :: a
    b = a + 5
    end function

end program
