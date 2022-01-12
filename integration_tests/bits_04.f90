program bits_04
    implicit none
    if (bgt(10, 4) .neqv. .true.) error stop
    if (bgt(10, -4) .neqv. .false.) error stop
    if (bgt(-10, 4) .neqv. .true.) error stop
    if (bgt(-10, -4) .neqv. .false.) error stop
    if (bgt(10, 0) .neqv. .true.) error stop
    if (bgt(-10, 0) .neqv. .true.) error stop
    if (bgt(10_8, 4_8) .neqv. .true.) error stop
    if (bgt(10_8, -4_8) .neqv. .false.) error stop
    if (bgt(-10_8, 4_8) .neqv. .true.) error stop
    if (bgt(-10_8, -4_8) .neqv. .false.) error stop
    if (bgt(10_8, 0_8) .neqv. .true.) error stop
    if (bgt(-10_8, 0_8) .neqv. .true.) error stop

    if (bge(10, 4) .neqv. .true.) error stop
    if (bge(10, -4) .neqv. .false.) error stop
    if (bge(-10, 4) .neqv. .true.) error stop
    if (bge(-10, -4) .neqv. .false.) error stop
    if (bge(10, 0) .neqv. .true.) error stop
    if (bge(-10, 0) .neqv. .true.) error stop
    if (bge(-10, -10) .neqv. .true.) error stop
    if (bge(10_8, 4_8) .neqv. .true.) error stop
    if (bge(10_8, -4_8) .neqv. .false.) error stop
    if (bge(-10_8, 4_8) .neqv. .true.) error stop
    if (bge(-10_8, -4_8) .neqv. .false.) error stop
    if (bge(10_8, 0_8) .neqv. .true.) error stop
    if (bge(-10_8, 0_8) .neqv. .true.) error stop
    if (bge(-10_8, -10_8) .neqv. .true.) error stop

    if (ble(10, 4) .neqv. .false.) error stop
    if (ble(10, -4) .neqv. .true.) error stop
    if (ble(-10, 4) .neqv. .false.) error stop
    if (ble(-10, -4) .neqv. .true.) error stop
    if (ble(10, 0) .neqv. .false.) error stop
    if (ble(-10, 0) .neqv. .false.) error stop
    if (ble(-10, -10) .neqv. .true.) error stop
    if (ble(10_8, 4_8) .neqv. .false.) error stop
    if (ble(10_8, -4_8) .neqv. .true.) error stop
    if (ble(-10_8, 4_8) .neqv. .false.) error stop
    if (ble(-10_8, -4_8) .neqv. .true.) error stop
    if (ble(10_8, 0_8) .neqv. .false.) error stop
    if (ble(-10_8, 0_8) .neqv. .false.) error stop
    if (ble(-10_8, -10_8) .neqv. .true.) error stop

    if (blt(10, 4) .neqv. .false.) error stop
    if (blt(10, -4) .neqv. .true.) error stop
    if (blt(-10, 4) .neqv. .false.) error stop
    if (blt(-10, -4) .neqv. .true.) error stop
    if (blt(10, 0) .neqv. .false.) error stop
    if (blt(-10, 0) .neqv. .false.) error stop
    if (blt(10_8, 4_8) .neqv. .false.) error stop
    if (blt(10_8, -4_8) .neqv. .true.) error stop
    if (blt(-10_8, 4_8) .neqv. .false.) error stop
    if (blt(-10_8, -4_8) .neqv. .true.) error stop
    if (blt(10_8, 0_8) .neqv. .false.) error stop
    if (blt(-10_8, 0_8) .neqv. .false.) error stop
end program