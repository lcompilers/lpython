subroutine g
x = y
x = 2*y
end subroutine

subroutine g
x = y

x = 2*y
end subroutine

subroutine g

    x = y


    x = 2*y



end subroutine

subroutine g
x = y
;;;;;; ; ; ;
x = 2*y
end subroutine

subroutine g
x = y;
x = 2*y;
end subroutine

subroutine g
x = y; ;
x = 2*y;; ;
end subroutine

subroutine g; x = y; x = 2*y; end subroutine

subroutine f
subroutine = y
x = 2*subroutine
end subroutine

subroutine f
integer :: x
end subroutine

subroutine f()
integer :: x
end subroutine

subroutine f(a, b, c, d)
integer, intent(in) :: a, b
integer, intent ( in ) :: c, d
integer :: z
integer::y
end subroutine

subroutine f(a, b, c, d)
integer, intent(out) :: a, b
integer, intent(in out) :: c, d
integer :: z
integer::y
end subroutine

subroutine saxpy(n, a, x, y)
real(dp), intent(in) :: x(:), a
real(dp), intent(inout) :: y(:)
integer, intent(in) :: n
integer :: i
do i = 1, n
    y(i) = a*x(i)+y(i)
enddo
end subroutine saxpy
