program if1
    integer :: i
    i = 5
    if (i == 5) print *, 'correct'
    if (i == 6) print *, 'incorrect'
    i = -2
40  i = i + 1
    if (i) 50, 60, 70
50  print *, 'i < 0'
    go to 40
60  print *, 'i == 0'
    go to 40
70  print *, 'i > 0'
end program
