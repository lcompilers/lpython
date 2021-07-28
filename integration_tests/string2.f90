program character1
implicit none

   character(len = 15) :: surname, firstname 
   character, allocatable :: title(:)
   ! character(len = :), allocatable :: greetings

   title = 'Mr. '
   firstname = 'Rowan '
   surname = 'Atkinson'
   allocate(title(6))
   ! greetings = 'A big hello from Mr. Bean'

   print *, 'Here is ', title, firstname, surname
   ! print *, greetings

end program character1
