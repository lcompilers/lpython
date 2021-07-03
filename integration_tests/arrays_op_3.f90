program array_op_3
implicit none

integer :: a(2, 2, 1), b(2, 2, 1)
logical :: c(2, 2, 1)
integer :: i, j, k

do i = 1, 2
    do j = 1, 2
        do k = 1, 1
            a(i, j, k) = i/j
            b(i, j, k) = j/i
        end do
    end do
end do

c = a .eq. b
call check(c, a, b, 0)

c = a .ne. b
call check(c, a, b, 1)

c = a .lt. b
call check(c, a, b, 2)

c = b <= a
call check(c, a, b, 3)

c = b .gt. a
call check(c, a, b, 4)

c = b >= a
call check(c, a, b, 5)

contains

    subroutine check(c, a, b, op_code)
    implicit none

    integer, intent(in) :: a(:, :, :), b(:, :, :)
    logical, intent(in) :: c(:, :, :)
    integer, intent(in) :: op_code

    integer :: i, j, k

    do i = lbound(a, 1), ubound(a, 1)
        do j = lbound(a, 2), ubound(a, 2)
            do k = lbound(a, 3), ubound(a, 3)
                select case(op_code)
                case (0)
                    if(c(i, j, k) .neqv. (a(i, j, k) .eq. b(i, j, k))) error stop
                case (1)
                    if(c(i, j, k) .neqv. (a(i, j, k) .ne. b(i, j, k))) error stop
                case (2)
                    if(c(i, j, k) .neqv. (a(i, j, k) .lt. b(i, j, k))) error stop
                case (3)
                    if(c(i, j, k) .neqv. (b(i, j, k) <= a(i, j, k))) error stop
                case (4)
                    if(c(i, j, k) .neqv. (b(i, j, k) .gt. a(i, j, k))) error stop
                case (5)
                    if(c(i, j, k) .neqv. (b(i, j, k) >= a(i, j, k))) error stop
                end select
            end do
        end do
    end do

    end subroutine check

end program