program intrinsics_13
 integer, parameter :: p = kind(5) ! 4
 integer, parameter :: p1 = kind(5_4) ! 4
 integer, parameter :: p2 = kind(5_8) ! 8
 integer, parameter :: p3 = kind(0.d0) ! 8
 integer, parameter :: p4 = kind(0.0) ! 4
 integer, parameter :: p5 = kind(5._4) ! 4
 integer, parameter :: p6 = kind(5._8) ! 8
 integer, parameter :: p7 = kind(.true.) ! 4
end program
