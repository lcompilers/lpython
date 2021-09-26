program goto_03
implicit none
integer :: a, n
n = 2
a = 10

go to (1, 2, 3), n

1 a = a + 10
2 a = a + 20
3 a = a + 30

if(a /= 60) error stop

assign 30 to n

go to n, (10, 20, 30)
10 a = a * a
20 a = a + a
30 a = a - a

if(a /= 0) error stop

end program
