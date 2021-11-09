program write2
implicit none
integer :: i
write(unit=*, fmt='(a)', advance='no') "Some text: "
read *, i
print *, "Got: ", i
end program
