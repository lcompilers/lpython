program do3
implicit none
integer :: i, j
j = 0
do 15 i = 1, 5
    j = j + i
15 enddo

j = 0
do 20 i = 1, 6, 2
    j = j + i
20 end do
end program
