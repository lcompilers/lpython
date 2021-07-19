program builtin_01
! AST only tests
implicit none

inquire(unit=n, opened=inuse)
INQUIRE (UNIT = JOAN, OPENED = LOG_01, NAMED = LOG_02, &
                FORM = CHAR_VAR, IOSTAT = IOS)
INQUIRE (IOLENGTH = IOL) A (1:N)

rewind(s)
rewind s
rewind 10
rewind(err=label, unit=s)

backspace (u)
backspace io_unit
backspace 10
BACKSPACE (10, IOSTAT = N)

END FILE K
end file 5
endfile (k)
endfile (10, iostat = n)
endfile(unit=iout,iostat=ios,iomsg=msg)

end program
