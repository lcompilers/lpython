module access_vars  
implicit none 

   public   
   real, private :: priv  = 1.5  
   real, public :: publ = 2.5 
   
contains      
   subroutine print_vars()          
      print *, "priv = ", priv
      print *, "publ = ", publ 
   end subroutine print_vars
   
end module access_var
