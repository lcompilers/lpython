program arrays_01_logical
implicit none
integer :: i, j
logical :: a(3), b(4), c(2, 2)

a(1) = .true.
do i = 2, 3
    a(i) = .not. a(i - 1)
end do
if (.not. a(1)) error stop
if (a(2)) error stop
if (.not. a(3)) error stop

b(1) = .false.
do i = 12, 14
    b(i-10) = .not. b(i - 10 - 1)
end do
if (b(1)) error stop
if (.not. b(2)) error stop
if (b(3)) error stop
if (.not. b(4)) error stop

do i = 1, 3
    b(i) = a(i) .and. .false.
end do
if (b(1)) error stop
if (b(2)) error stop
if (b(3)) error stop

b(4) = b(1) .or. b(2) .or. b(3) .or. a(1)
if (.not. b(4)) error stop

b(4) = a(1)
if (.not. b(4)) error stop

do i = 1, 2
    do j = 1, 2
        if (((i + j) - 2*((i + j)/2)) == 1) then
            c(i, j) = .true.
        else
            c(i, j) = .false.
        end if
    end do
end do
if (c(1, 1)) error stop
if (.not. c(1, 2)) error stop
if (.not. c(2, 1)) error stop
if (c(2, 2)) error stop
end
