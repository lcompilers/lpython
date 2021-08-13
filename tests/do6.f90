program do6
implicit none
integer :: i, j, k

k = 0
do 30 i = 1, 5
    do 30 j = 1, 5
30 k = k + 1
write(*,*) k

k = 0
do 35 i = 1, 5
    do 35 j = 1, 5
        k = k + 1
35 continue
write(*,*) k

k = 0
do 40 i = 1, 25
    k = k + 1
40 enddo
write(*,*) k

k = 0
do 45 i = 1, 25
    k = k + 1
45 end do
write(*,*) k

k = 0
do 50 i = 1, 5
    do 50 j = 1, 5
        k = k + 0
50 k = k + 1
write(*,*) k

k = 0
do 60 i = 1, 25
    k = k + 0
60 k = k + 1
write(*,*) k

k = -25
do 65 i = 1, 5
    do 65 j = 1, 5
        k = k + 2
65 continue
write(*,*) k

end program