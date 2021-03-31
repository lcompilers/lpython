program complex_dp

    complex(4) :: zero
    complex(8) :: v
    complex :: x
    zero = 0.0_4
    v = (1.05_4, 1.05_4)
    x = (1.05_4, 1.05_8)
    print *, v, x, zero

end program