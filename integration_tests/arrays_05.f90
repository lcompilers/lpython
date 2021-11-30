program arrays_05
implicit none
real, dimension(5) :: numbers = [1.5, 3.2, 4.5, 0.9, 7.2]
integer :: i
do i = 1, 5
   numbers(i) = i * 2.0
end do
numbers = [1.5, 3.2, 4.5, 0.9, 7.2]
end program
