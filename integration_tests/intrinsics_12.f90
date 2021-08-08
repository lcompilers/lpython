program intrinsics_12
print*,kind(5) ! 4
print*,kind(5_4) ! 4
print*,kind(5_8) ! 8
print*,kind(0.d0) ! 8
print*,kind(0.0) ! 4
print*,kind(5._4) ! 4
print*,kind(5._8) ! 8
print*,kind(.true.) ! 4
end program
