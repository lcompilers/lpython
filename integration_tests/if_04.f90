program if_04
implicit none
integer :: i
i = 1
if (i == 1) then
    print *, "1"
else if (i == 2) then
    print *, "2"
elseif (i == 3) then
    print *, "3"
else if (i == 4) then
    print *, "4"
end if

name: if (i == 1) then
    print *, "1"
else if (i == 2) then name
    print *, "2"
elseif (i == 3) then name
    print *, "3"
else if (i == 4) then name
    print *, "4"
else name
    print *, "Invalid!"
endif name

end
