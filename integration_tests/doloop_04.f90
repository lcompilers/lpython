program doloop_04
implicit none
integer :: i, j
j = 0
a: do i = 1, 10
    j = j + i
    if (i == 2) exit a
end do a
if (j /= 3) error stop

j = 0
b: do i = 1, 10, 2
    j = j + i
    if (i == 3) exit b
end do b
if (j /= 4) error stop

j = 0
i = 1
c: do
    j = j + i
    if (i == 2) exit c
    i = i + 1
end do c
if (j /= 3) error stop

end
