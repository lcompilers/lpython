program string_04
implicit none

character(len = 40), allocatable :: user_data(:)
character, allocatable :: greetings(:)

allocate(user_data(3))
user_data(1) = 'Mr. '
user_data(2) = 'Rowan '
user_data(3) = 'Atkinson'
allocate(greetings(5))
greetings(1) = 'h'
greetings(2) = 'e'
greetings(3) = 'l'
greetings(4) = 'l'
greetings(5) = 'o'

print *, 'Here is ', user_data(1), user_data(2), user_data(3)
print *, greetings

end program
