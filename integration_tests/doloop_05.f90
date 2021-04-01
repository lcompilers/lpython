program doloop_05
implicit none
integer :: i, j
j = 0
i_loop: do i = 1, 10
    if (i == 2) cycle i_loop
    j = j + i
end do i_loop
if (j /= 53) error stop
end
