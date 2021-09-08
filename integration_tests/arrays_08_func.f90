program arrays_08_func
implicit none
integer :: x(10), y(10), i
logical :: r
do i = 1, size(x)
    x(i) = i
end do
call copy_from_to(x, y)
r = verify(x, y)
print *, r
if (.not. r) error stop

contains

    subroutine copy_from_to(a, b)
    integer, intent(in) :: a(:)
    integer, intent(out) :: b(:)
    integer :: i
    do i = 1, size(a)
        b(i) = a(i)
    end do
    end subroutine

    logical function verify(a, b) result(r)
    integer, intent(in) :: a(:), b(:)
    integer :: i
    r = .true.
    do i = 1, size(a)
        r = r .and. (a(i) .eq. b(i))
    end do
    end function

end

