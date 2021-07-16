program allocate_03
implicit none

    integer, allocatable :: c(:, :, :)
    allocate(c(3, 3, 3))
    c(1, 1, 1) = 3
    print *, g(c)
    print *, c(1, 1, 1)
    call h(c)
    print *, c(1, 1, 1)

contains

subroutine f(c)
    integer, allocatable, intent(out) :: c(:, :, :)
    allocate(c(3, 3, 3))
    c(1, 1, 1) = 2
end subroutine

function g(c) result(r)
    integer, allocatable :: c(:, :, :)
    integer :: r
    call f(c)
    print *, c(1, 1, 1)
    c(1, 1, 1) = 1
    r = 0
end function

subroutine h(c)
    integer, allocatable, intent(out) :: c(:, :, :)
    call f(c)
    print *, c(1, 1, 1)
    c(1, 1, 1) = 1
end subroutine

end program
