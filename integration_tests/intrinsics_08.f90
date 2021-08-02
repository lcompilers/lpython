program intrinsics_08
  real, parameter :: x=tiny(1.0)
  real (kind = 8) :: b
  real (kind = 8), parameter :: y=tiny(b)
  print*, x
  print*, y
  ! Not implemented
  ! real, parameter :: x=tiny([1.0, 2.0])
end program
