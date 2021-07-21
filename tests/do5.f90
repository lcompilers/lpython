program do4
implicit none
integer :: i, j, k
j = 0
do 15 i = 1, 5
15 j = j + i

j = 0
do 20 i = 1, 6, 2
20 j = j + i

k = 0
do 30 i = 1, 5
    do 35 j = 1, 5
31 continue
35  k = k + 1
33 continue
30 k = k + 1
40 continue
end program
