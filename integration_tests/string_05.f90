program string_05
character(len=3) :: s1
character(len=5) :: s2
character(len=7) :: s3

print *, len(s1)
print *, len(s2)
print *, len(s3)
if (len(s1) /= 3) error stop
if (len(s2) /= 5) error stop
if (len(s3) /= 7) error stop

end program
