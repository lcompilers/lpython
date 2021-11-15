program debug
    implicit none
    integer, parameter :: a = 10, b = -20, c = 30
    integer, parameter :: d = -40, e = 0, f = 50
    print *, min(a, b, c, d, e, f)
    print *, max(a, b, c, d, e, f)
    print *, max(a, b)
    print *, min(a, b)
end program