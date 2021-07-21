program array_op_3
implicit none

logical, allocatable :: a(:, :, :), b(:, :, :)
logical, allocatable :: c(:, :, :)
integer :: i, j, k, dim1 = 10, dim2 = 100, dim3 = 1

allocate(a(dim1, dim2, dim3), b(dim1, dim2, dim3), c(dim1, dim2, dim3))

do i = 1, dim1
    do j = 1, dim2
        do k = 1, dim3
            a(i, j, k) = modulo2(i + j + k)
            b(i, j, k) = modulo2(i*j + j*k + k*j)
        end do
    end do
end do

c = a .and. b
call verify(c, 0)

c = a .or. b
call verify(c, 1)

c = a .eqv. b
call verify(c, 2)

c = b .neqv. a
call verify(c, 3)

contains

    logical function modulo2(x) result(r)
    integer, intent(in) :: x
    r = (x - 2*(x/2) == 1)
    end function modulo2

    subroutine verify(c, op_code)
    implicit none

    logical, allocatable, intent(in) :: c(:, :, :)
    integer, intent(in) :: op_code

    integer :: i, j, k
    logical :: x, y

    do i = lbound(c, 1), ubound(c, 1)
        do j = lbound(c, 2), ubound(c, 2)
            do k = lbound(c, 3), ubound(c, 3)
                x = modulo2(i + j + k)
                y = modulo2(i*j + j*k + k*j)
                select case(op_code)
                case (0)
                    if(c(i, j, k) .neqv. (x .and. y)) error stop
                case (1)
                    if(c(i, j, k) .neqv. (x .or. y)) error stop
                case (2)
                    if(c(i, j, k) .neqv. (x .eqv. y)) error stop
                case (3)
                    if(c(i, j, k) .neqv. (x .neqv. y)) error stop
                end select
            end do
        end do
    end do

    end subroutine verify

end program