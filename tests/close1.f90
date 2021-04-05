program close1
! Tests for syntax (AST) only:
close(u)
close (unit=gmv_lun)
close(unit=dxf % dunit, status='delete')
CLOSE( UNIT=funit, IOSTAT=ierr )
end program
