program do7
implicit none
integer :: i, a

do i = 1, 10
    a = f(5)
end do

contains

    integer function f(a)
    integer, intent(in) :: a
    f = a + 1
    end function

end program
