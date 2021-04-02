program arrays_01
implicit none
integer :: a(240)
integer, allocatable :: b(:, :)
a(218) = 25
print *, a(218)
end program
    