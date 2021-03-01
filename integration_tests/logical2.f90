program logical2
    ! this program checks logical operators
    implicit none
    
       ! variable declaration
       logical :: a, b
       
       ! assigning values
       a = .true.
       b = .false.

       if (a .and. b) then
          print *, "Line 1 - Condition is true"
       else
          print *, "Line 1 - Condition is false"
       end if
       
end program logical2