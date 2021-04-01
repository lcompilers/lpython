program complex_dp_param

    integer, parameter :: prec1 = 4, prec2 = 8
    complex(prec1) :: u = (1.05, 1.05)
    complex(prec2) :: v = (1.05, 1.05_8)
    complex(prec2) :: zero = 0.0_4
    print *, u, v, zero

end program