program interface_06
implicit none

interface a
    procedure a1
    procedure a2
end interface

integer :: i
real :: r

i = 5
i = a(i)
if (i /= 6) error stop

r = 6
r = a(r)
if (r /= 7) error stop

i = 7
i = a(i)
if (i /= 8) error stop

contains

    integer function a1(a)
    integer, intent(in) :: a
    a1 = a + 1
    end function

    real function a2(a)
    real, intent(in) :: a
    a2 = a + 1
    end function

end program
