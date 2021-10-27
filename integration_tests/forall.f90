program main
implicit none
integer :: ivec(3), i

forall(i=1:3) ivec(i)=i
print *,ivec

end program main
