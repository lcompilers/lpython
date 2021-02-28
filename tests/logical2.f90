program logical1
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
       
       if (a .or. b) then
          print *, "Line 2 - Condition is true"
       else
          print *, "Line 2 - Condition is false"
       end if
       
       ! changing values
       a = .false.
       b = .true.
       
       if (.not.(a .and. b)) then
          print *, "Line 3 - Condition is true"
       else
          print *, "Line 3 - Condition is false"
       end if
       
       if (b .neqv. a) then
          print *, "Line 4 - Condition is true"
       else
          print *, "Line 4 - Condition is false"
       end if
       
       if (b .eqv. a) then
          print *, "Line 5 - Condition is true"
       else
          print *, "Line 5 - Condition is false"
       end if
       
end program logical1