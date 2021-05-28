program array_bound_1

    integer :: a(2:5, 3:9, 7)
    integer :: b(1:2, 2:4, 3:6, 4:7)
    integer :: c(6:10, 7)
    integer :: d(4)

    print *, lbound(a, 1), lbound(a, 2), lbound(a, 3)
    print *, ubound(a, 1), ubound(a, 2), ubound(a, 3)
    
    print *, lbound(b, 1), lbound(b, 2), lbound(b, 3), lbound(b, 4)
    print *, ubound(b, 1), ubound(b, 2), ubound(b, 3), ubound(b, 4)

    print *, lbound(c, 1), lbound(c, 2)
    print *, ubound(c, 1), ubound(c, 2)

    print *, lbound(d, 1)
    print *, ubound(d, 1)

end program array_bound_1