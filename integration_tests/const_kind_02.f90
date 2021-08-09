program const_kind_02
integer, parameter :: int32 = 4, int64 = 8
integer(int32), parameter :: i1 = 1_int32
integer(int64), parameter :: i2 = 1_int64
integer :: i3
i3 = 1_int32
i3 = 1_int64
print *, int32, int64, i1, i2, i3
end program
