program array_op_5
implicit none

real :: a(2, 2, 2), b(2, 2, 2), c(2, 2, 2)
real :: d(2, 2, 2)
integer :: i, j, k

do i = 1, 2
    do j = 1, 2
        do k = 1, 2
            a(i, j, k) = i*i + j*j + k*k
            b(i, j, k) = 2*(i*j + j*k + i*k)
            c(i, j, k) = 0
        end do
    end do
end do

d = asquare(a, b, c)
call check(d)

contains

    function asquare(a, b, c) result(d)
        implicit none

        real :: a(:, :, :), b(:, :, :), c(:, :, :)
        real :: d(2, 2, 2)

        d = a + b + c

    end function asquare

    subroutine check(c)
    implicit none

    real, intent(in) :: c(:, :, :)

    integer :: i, j, k

    do i = lbound(c, 1), ubound(c, 1)
        do j = lbound(c, 2), ubound(c, 2)
            do k = lbound(c, 3), ubound(c, 3)
                if(c(i, j, k) /= (i + j + k)**2) error stop
            end do
        end do
    end do

    end subroutine check

end program