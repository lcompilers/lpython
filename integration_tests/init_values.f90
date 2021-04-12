program init_values

    integer, parameter :: i = 1.0, j = 2.0
    real, parameter :: r = 4.0
    complex, parameter :: c = (3.0, 4.0)
    integer, parameter :: a = i + j
    logical, parameter :: l = a == 3
    logical, parameter :: b = l .or. .true.
    real, parameter :: r_minus = -r
    character(len = 1), parameter :: s1 = "l"
    character(len = 3), parameter :: s2 = "eft"
    character(len = 4), parameter :: s = s1//s2
    print *, i, j, r, c, a, l, b, r_minus, s

end program
