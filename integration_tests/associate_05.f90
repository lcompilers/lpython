program associate_05

    real :: x(10), y(10), theta(10), a(10)
    real, allocatable :: myreal(:)
    allocate(myreal(10))
    x = 0.42
    y = 0.35
    myreal = 9.1
    theta = 1.5
    a = 0.4

    associate ( z => -(x*2 + y*2) * theta, v => myreal)
        v = a + z
        v(1) = v(1) * 4.6
        v(2:5) = v(2:5) * v(2:5)
        v(6:10) = v(6:10) * 2.6
    end associate

    print *, myreal 

end program associate_05