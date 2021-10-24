program substring
implicit none
character(5) :: str
character(5), parameter :: s = "Hello World"(7:11)

str = "12345"(2:4)

print *, str
print *, s
print *, "SubString"(4:9)

end program
