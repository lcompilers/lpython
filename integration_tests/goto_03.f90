program goto_03
implicit none
integer :: a, n
n = 2
a = 10

go to (1, 2, 3), n

1 a = a + 10
2 a = a + 20
3 a = a + 30
print *, a

if(a /= 60) error stop

end program
