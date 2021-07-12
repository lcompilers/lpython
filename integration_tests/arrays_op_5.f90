program array_op_5
implicit none

integer :: a(2, 2, 1), b(2, 2, 1)
complex :: c(2, 2, 1)
integer :: i, j, k

do i = 1, 2
    do j = 1, 2
        do k = 1, 1
            a(i, j, k) = i + j + k
            b(i, j, k) = i*j*k
        end do
    end do
end do


c = a + (0.0, 1.0)*b
call check(c, 0)

c = -a + (0.0, 1.0)*(-b)
call check(c, 1)

contains

    subroutine check(c, op_code)
    implicit none

    complex, intent(in) :: c(:, :, :)
    integer, intent(in) :: op_code

    integer :: i, j, k
    real :: left, right

    do i = lbound(c, 1), ubound(c, 1)
        do j = lbound(c, 2), ubound(c, 2)
            do k = lbound(c, 3), ubound(c, 3)
                select case(op_code)
                    case (0)
                        left = i + j + k
                        right = i*j*k
                        if(c(i, j, k) /= left + right*(0.0, 1.0)) error stop
                    case (1)
                        left = i + j + k
                        right = i*j*k
                        if(c(i, j, k) /= -left  - right*(0.0, 1.0)) error stop
                end select
            end do
        end do
    end do

    end subroutine check

end program