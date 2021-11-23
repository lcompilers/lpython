program intrinsics_27
    integer, dimension(-1:1, -1:2) :: a
    print *, shape(a)             ! (/ 3, 4 /)
    print *, size(shape(42))      ! (/ /)
end program
