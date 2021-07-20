program read1
! AST only tests
read(*,*) a, b
read(unit=*,fmt=*) a, b
read(*,fmt=*) a, b
read(*) b
read(zone(1:3), '(i3)') hour
read(u) b
read(u, fmt=x) b, c, e
read(*, fmt=x) b, c, e
READ( iunit, NML=invar, IOSTAT=ierr )
READ( UNIT=iunit, NML=invar, IOSTAT=ierr )
read 10
READ 10, A, B
read *, size
end program
