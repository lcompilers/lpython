program string_02
implicit none

character(len = 15) :: surname, firstname
character(len = 6) :: title
character(len = 25)::greetings

title = 'Mr. '
firstname = 'Rowan '
surname = 'Atkinson'
greetings = 'A big hello from Mr. Bean'

print *, 'Here is ', title, firstname, surname
print *, greetings

end program
