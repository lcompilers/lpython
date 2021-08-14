program do6
implicit none
integer :: i, j, k
integer :: correct

correct = 0
k = 0
do 30 i = 1, 5
    do 30 j = 1, 5
30 k = k + 1
write(*,*) k
correct = correct + k

k = 0
do 35 i = 1, 5
    do 35 j = 1, 5
        k = k + 1
35 continue
write(*,*) k
correct = correct + k

k = 0
do 40 i = 1, 25
    k = k + 1
40 enddo
write(*,*) k
correct = correct + k

k = 0
do 45 i = 1, 25
    k = k + 1
45 end do
write(*,*) k
correct = correct + k

k = 0
do 50 i = 1, 5
    do 50 j = 1, 5
        k = k + 0
50 k = k + 1
write(*,*) k
correct = correct + k

k = 0
do 60 i = 1, 25
    k = k + 0
60 k = k + 1
write(*,*) k
correct = correct + k

k = -30
do 65 i = 1, 5
    k = k + 1
    do 65 j = 1, 5
        if (k == 25) go to 70
        k = k + 2
65 continue
write(*,*) k
correct = correct + k

if (k == 25) then
    j = 0
    go to 65
end if

70 continue
write(*,*) k
correct = correct + k

k = -30
do 75 i = 1, 5
    k = k + 1
    do 75 j = 1, 5
        if (k == 25) go to 80
        k = k + 2
75 end do
write(*,*) k
correct = correct + k

if (k == 25) then
    j = 0
    go to 75
end if

80 continue
write(*,*) k
correct = correct + k

write(*, *) 10*correct/25, '% correct'

end program