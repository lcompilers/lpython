program case03
    implicit none

    ! local variable declaration
    integer :: marks
    integer, parameter :: a = 1, b = 2
    marks = 94
 
    select case (marks)
    
       case ((40 + b):) 
          print *, "Pass!"
 
       case (:(39 - a))
          print *, "Failed!"
       
       case default
          print*, "Invalid marks" 
          
    end select
    print*, "Your marks are ", marks

    marks = -1
    
    select case (marks)
    
      case ((40 + b):) 
         print *, "Pass!"

      case (0:(39 - a))
         print *, "Failed!"
      
      case default
         print*, "Invalid marks" 
         
   end select
   print*, "Your marks are ", marks
    
end program
