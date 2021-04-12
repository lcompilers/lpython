program functions_03
implicit none
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
j = f((2.0, 0.0))
print *, j

j = 1
print *, j
j = f((2.0, 3.0))
print *, j

contains

    complex(4) function f(a) result(b)
    complex(4), intent(in) :: a
    b = a + 1
    end function

end program
