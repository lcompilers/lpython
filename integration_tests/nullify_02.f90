program nullify_02
implicit none

   real, pointer :: p1
   real, target :: t1 
   integer, pointer :: p2
   integer, target :: t2
   
   p1=>t1
   p1 = 1
   p2=>t2
   p2 = 2.
   nullify(p1, p2) 
   
   
end program nullify_02
