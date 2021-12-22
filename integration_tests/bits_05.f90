program bits_05
    implicit none
    integer(4) :: from, res
    integer(8) :: from8, res8
    from = 10
    from8 = 10_8

    res = ibits(from, 2, 2)
    if (res /= 2) error stop

    res8 = ibits(from8, 2, 2)
    if (res8 /= 2_8) error stop

    res = ibits(from, 0, 2)
    if (res /= 2) error stop

    res8 = ibits(from8, 0, 2)
    if (res8 /= 2_8) error stop

    from  = -20
    from8 = -20_8

    res = ibits(from, 29, 2)
    if (res /= 3) error stop

    res8 = ibits(from8, 29, 2)
    if (res8 /= 3) error stop

    res = ibits(from, 2, 2)
    if (res /= 3) error stop

    res8 = ibits(from8, 2, 2)
    if (res8 /= 3) error stop

end program