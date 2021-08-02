program intrinsics_09
  real :: z=0.0
  if (z < tiny(1.0)) then
     print*, z
  end if
end program
