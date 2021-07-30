! Reference - https://www.ibm.com/docs/en/xffbg/121.141?topic=control-associate-construct-fortran-2003
program associate_04

    real :: myreal, x, y, theta, a
    x = 0.42
    y = 0.35
    myreal = 9.1
    theta = 1.5
    a = 0.4

    associate ( z => -(x*2 + y*2) * cos(theta), v => myreal)
        print *, a + z, a - z, v
        v = v * 4.6
    end associate

    print *, myreal

    if ((myreal - 41.86) > 1e-5 .or. myreal - 41.86 < -1e-5) error stop 

end program associate_04