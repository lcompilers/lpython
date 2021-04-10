program doloop_05
implicit none
integer :: i, j
1 j = 0
2 i_loop: do i = 1, 10
3    if (i == 2) cycle i_loop
4    j = j + i
5 end do i_loop
6 if (j /= 53) error stop
end
