program forall_01
implicit none
integer :: ivec(3), i

forall(i=1:3) ivec(i)=i
print *,ivec

end program forall_01
