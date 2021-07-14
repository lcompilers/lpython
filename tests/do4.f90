program do4
implicit none
integer :: i, j, k
j = 0
do 15 i = 1, 5
    j = j + i
15 continue

j = 0
do 20 i = 1, 6, 2
    j = j + i
20 continue

k = 0
do 30 i = 1, 5
    do 35 j = 1, 5
        k = k + 1
31 continue
35 continue
33 continue
30 continue
40 continue
end program
