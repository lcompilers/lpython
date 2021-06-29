program arrays_09
implicit none
character(10) :: a(1)
a(1) = 'Substring'
if(a(1)(4:9) /= 'string') error stop
end program
