program line_continuation_01
implicit none
integer :: i
i = 5
if (i < 4) &
    print *, i
if (i > 4) & ! This will print
    print *, i
if (i > 4) &
    ! This will also print
    print *, i
if (i > 4) & ! Yes
    ! This will also print
    print *, i
end program
