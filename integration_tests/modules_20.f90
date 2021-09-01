program modules_20
use modules_20b, only: trim2
implicit none
character(*), parameter :: s1 = " A B ", s2 = " "

call trim2(s1)
call trim2(s2)
call trim2("xx    ")

end program
