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

do, i = 1, 5
    a = a + i
end do

n = 0
do 50, i = 1, 10
  j = i
  do k = 1, 5
    l = k
    n = n + 1  ! this statement executes 50 times
  end do       ! nonlabeled do inside a labeled do
50 continue

end subroutine
