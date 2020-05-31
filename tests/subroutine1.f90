subroutine g()
integer :: x, i
x = 1
do i = 1, 10
    x = x*i
end do
end subroutine

subroutine h()
integer :: x, i
x = 1
do i = 1, 10
    x = i*x
end do
end subroutine
