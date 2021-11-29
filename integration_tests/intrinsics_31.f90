program intrinsics_31
    real :: x = 63.29
    real :: y = -63.59
    real :: x_ceil, y_ceil
    x_ceil = ceiling(x)
    y_ceil = ceiling(y)
    print *, x_ceil, ceiling(x_ceil)
    print *, y_ceil, ceiling(y_ceil)
end program intrinsics_31