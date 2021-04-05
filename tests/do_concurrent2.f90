subroutine do_concurrent2(a, b)
real, intent(inout) :: a(:), b(:)
integer :: i
real :: x
x = 1.0
do concurrent (i=1:10) shared(i) local(x) default(none)
    if (a(i) > 0) then
        x = sqrt(a(i))
        a(i) = a(i) - x**2
    end if
    b(i) = b(i) - a(i)
end do
print *, x
end subroutine
