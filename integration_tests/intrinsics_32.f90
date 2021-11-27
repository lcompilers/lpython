program intrinsics_32
    implicit none

    real, dimension(6) :: tsource, fsource, result
    logical, dimension(6) :: mask
    integer :: i

    do i = 1, 6
        tsource(i) = i*2
        fsource(i) = -i*2
    end do
    mask(1) = .true.
    mask(2) = .false.
    mask(3) = .false.
    mask(4) = .true.
    mask(5) = .true.
    mask(6) = .false.

    result = merge(tsource, fsource, mask)
    print *, result
end program intrinsics_32
