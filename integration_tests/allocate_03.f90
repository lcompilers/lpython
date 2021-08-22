program allocate_03
implicit none

    integer, allocatable :: c(:, :, :)
    integer :: r
    integer :: stat
    stat = 1
    allocate(c(3, 3, 3), STAT=stat)
    if (stat /= 0) error stop
    c(1, 1, 1) = 3
    call h(c)
    r = g(c)
    print *, c(1, 1, 1)

contains

subroutine f(c)
    integer, allocatable, intent(out) :: c(:, :, :)
    allocate(c(3, 3, 3))
    c(1, 1, 1) = 99
end subroutine

function g(x) result(r)
    integer, allocatable :: x(:, :, :)
    integer :: r
    print *, x(1, 1, 1)
    call f(x)
    print *, x(1, 1, 1)
    x(1, 1, 1) = 8
    r = 0
end function

subroutine h(c)
    integer, allocatable, intent(out) :: c(:, :, :)
    print *, c(1, 1, 1)
    call f(c)
    print *, c(1, 1, 1)
    c(1, 1, 1) = 8
end subroutine

end program
