do i = 1, 5
    a = a + i
end do

do i = 1, 5
end do

do i = 1, 5, 2
    a = a + i
end do

do
    a = a + i
    b = 3
end do

subroutine g
do
    a = a + i
    b = 3
enddo
end subroutine
