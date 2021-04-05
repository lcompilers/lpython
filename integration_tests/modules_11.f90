module module_11  
implicit none 
integer :: i = 1
integer :: j = 2

contains      

   subroutine access_internally()          
      print*, "i = ", i
   end subroutine access_internally 
   
end module module_11

program access_externally
use module_11
implicit none
   print*, "j = ", j
   call access_internally() 
   
end program access_externally
