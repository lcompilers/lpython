program string_09
implicit none
character(*), parameter :: s1 = " A B ", s2 = " "

if (len_trim(s1) /= 4) error stop
if (len_trim(s2) /= 0) error stop
if (len_trim("  ") /= 0) error stop
if (len_trim("") /= 0) error stop
if (len_trim("xx") /= 2) error stop

if (trim(s1) /= " A B") error stop
if (trim(s2) /= "") error stop
if (trim("  ") /= "") error stop
if (trim("") /= "") error stop
if (trim("xx") /= "xx") error stop

if (len(trim(s1)) /= 4) error stop
if (len(trim(s2)) /= 0) error stop
if (len(trim("  ")) /= 0) error stop
if (len(trim("")) /= 0) error stop
if (len(trim("xx")) /= 2) error stop

print *, trim("xx    ")
print *, len(trim("xx    "))

end program
