program bits_03
    implicit none
    integer(4) :: from, to
    integer(8) :: from8, to8
    from = 10
    to = 4
    from8 = 10_8
    to8 = 4_8
    call mvbits(from, 2, 2, to, 0)
    if (from /= 10) error stop
    if (to /= 6) error stop
    call mvbits(from8, 2, 2, to8, 0)
    if (from8 /= 10) error stop
    if (to8 /= 6_8) error stop
    call mvbits(from, 0, 2, to, 2)
    if (from /= 10) error stop
    if (to /= 10) error stop
    call mvbits(from8, 0, 2, to8, 2)
    if (from8 /= 10) error stop
    if (to8 /= 10_8) error stop
    from  = -20
    to = 4
    from8 = -20_8
    to8 = 4_8
    call mvbits(from, 29, 2, to, 2)
    if (from /= -20) error stop
    if (to /= 12) error stop
    call mvbits(from8, 29, 2, to8, 2)
    if (from8 /= -20) error stop
    if (to8 /= 12_8) error stop
    call mvbits(from, 2, 2, to, 29)
    if (from /= -20) error stop
    if (to /= 1610612748) error stop
    call mvbits(from8, 2, 2, to8, 29)
    if (from8 /= -20) error stop
    if (to8 /= 1610612748_8) error stop
end program