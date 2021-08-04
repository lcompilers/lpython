subroutine a1(n, A, B)
integer, intent(in) :: n
real, intent(in) :: A(n)
real, intent(out) :: B(n)
integer :: i
!$OMP PARALLEL DO
do i = 2, N
    B(i) = (A(i) + A(i-1)) / 2
end do
!$OMP END PARALLEL DO
end subroutine
