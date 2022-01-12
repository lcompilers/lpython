program intrinsics_29
implicit none

real(4) :: random_sp
real(8) :: random_dp

call random_number(random_sp)
call random_number(random_dp)
print*, random_sp, random_dp

end program intrinsics_29
