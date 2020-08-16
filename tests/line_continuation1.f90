program line_continuation1
implicit none
integer :: i
i = 5
if (i < 4) &
    print *, i
if (i > 4) & ! This will print
    print *, i
end program
