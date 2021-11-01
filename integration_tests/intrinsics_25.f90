program intrinsics_25
implicit none

integer(4), parameter:: x = ishft(123, 5)
integer(4), parameter:: y = ishft(25, 0)
integer(8), parameter:: z = ishft(1000000000, -2)

print *, x, y, z
print *, ishft(123, 5), ishft(25, 0), ishft(1000000000, -2)

end program intrinsics_25
