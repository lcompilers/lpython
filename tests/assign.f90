program assign
    integer :: x
    real :: y
10  format('(i3)')
    assign 10 to x
20  assign 10 to x
    ASSIGN 10 TO x
30  ASSIGN 10 TO y ! An integer variable is required in this context.
end program
