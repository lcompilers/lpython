subroutine a
do
    a = a + i
    b = 3
enddo

do
    do = a + i
    enddo = 3
enddo

do; a = a + i; b = 3; end do
end subroutine

subroutine a
do
x = 1
end do
end subroutine

subroutine a
do i = 1, 5
x = x + i
end do
end subroutine

subroutine a
do i = 1, 5, -1
x = i
end do
end subroutine
