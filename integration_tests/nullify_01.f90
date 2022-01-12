program nullify_01
implicit none

   integer, pointer :: p1, p2
   integer, target :: t1 
   
   p1=>t1
   p2=>t1
   p1 = 1
   nullify(p1, p2)
   
end program nullify_01
