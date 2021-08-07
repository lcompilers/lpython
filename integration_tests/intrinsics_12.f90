program intrinsics_12
print*,kind(5) ! 4 Y
print*,kind(5_4) ! 4 Y
print*,kind(5_8) ! 8 N
print*,kind(0.d0) ! 8 N
print*,kind(0.0) ! 4 Y
print*,kind(5._4) ! 4 Y
print*,kind(5._8) ! 8 Y
print*,kind(.true.) ! 4
end program
