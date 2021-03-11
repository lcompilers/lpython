program case03
    implicit none

    ! local variable declaration
    integer :: marks = 38
    integer, parameter :: a = 1, b = 2
 
    select case (marks)
    
       case ((40 + b):) 
          print *, "Pass!"
 
       case (:(39 - a))
          print *, "Failed!"
       
       case default
          print*, "Invalid marks" 
          
    end select
    print*, "Your marks are ", marks
    
end program
