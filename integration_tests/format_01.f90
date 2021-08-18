program format_01
implicit none
    1 format(/,'BDF2 step ',i6,': T=',es12.5,', H=',es12.5,', ETAH=',es12.5)
integer :: x
    2 format(/ ,'BDF2 step ',i6,': T=',es12.5,', H=',es12.5,', ETAH=',es12.5)
    3 format( /,'BDF2 step ',i6,': T=',es12.5,', H=',es12.5,', ETAH=',es12.5)
    4 format(2x,'Preconditioner update FAILED at T=',es12.5,', ETAH=',es12.5)
    5 format(/)
    6 format(/ )
    7 format( /)
    8 format(i6, /)
    9 format(i6,/)
    10 format(/ , /)
    121 FORMAT( /, 80A, / )
integer :: y
    122 FORMAT( /, 80A, /)
    123 FORMAT(/, 80A, /)
    124 FORMAT(/, 80A, / )
    130 FORMAT( 10X,                                                   &
               'keyword Input Echo - Values from input or default', / ,&
                80A, / )
    131 FORMAT( 10X,                                                   &
               'keyword Input Echo - Values from input or default', / ,&
                80A, /)
    158 FORMAT( 4X, 'epsi = ', ES11.4, /, 4X, 'iitm = ', I3, /,        &
                4X, 'oitm = ', I4, /, 4X, 'timedep = ', I1, /,         &
                4X, 'swp_typ = ', I1, /, 4X, 'multiswp = ', I1, /,     &
                4X, 'angcpy = ', I1, /,  4X, 'it_det = ', I1, /,       &
                4X, 'soloutp = ', I1, /, 4X, 'kplane = ', I4, /,       &
                4X, 'popout = ', I1, /, 4X, 'fluxp = ', I1, /,         &
                4X, 'fixup = ', I1, / )
    161 FORMAT( 'slgg(nmat,nmom,ng,ng) echo', /, 'Column-order loops:',&
                ' Mats (fastest ), Moments, Groups, Groups (slowest)' )
    162 FORMAT( 2X, ES15.8, 2X, ES15.8, 2X, ES15.8, 2X, ES15.8 )
    221 FORMAT( 4X, 'Group ', I3, 4X, ' Inner ', I5, 4X, ' Dfmxi ',    &
                ES11.4, '     Fixup x/y/z/t ', 4(ES9.2,1x) )
    306 FORMAT( 2X, I4, 6(1X, ES11.4) )
    324 FORMAT( 4(2X, ES17.10) )
    422 FORMAT( /, 4X, 'Manufactured/Computed Solutions Max Diff=',    &
                 ES13.6 )
    500 format( "IARRAY =", *( I0, :, ","))
    501 format(*( I0, :, ","))
    510 FORMAT (1X, F10.3, I5, F10.3, I5/F10.3, I5, F10.3, I5)
    511 FORMAT (3/,I5)
    512 format(3/)
    513 FORMAT (1x, A, 3/ 1x, 2(I10,F10.2) // 1x, 2E15.7)
    520 FORMAT (1X, I1, 1X, 'ISN''T', 1X, I1)
    530 FORMAT (1PE12.4, I10)
    540 FORMAT (I12, /, ' Dates: ', 2 (2I3, I5))
    550 FORMAT (ES12.3, ES12.3E3, G3.4, G3.4E100)
    600 format(//,63x,'Internal',/, &
           1x,'Cell',3(5x,'Temp '), 7x,'P',8x,'Density',6x,'Energy', &
           /,2x,'Num',6x,'(K)',7x,'(C)',7x,'(F)',6x,'(Pa)',6x, &
           '(kg/m**3)',5x,'(J/kg)' )
    610 format(71('-'),/,(1x,i4,0p,3f10.1,1p,3e12.3))
    620 format((1x,i4,0p,3(2x,'|',2x,f5.1),1p, 3(1x,'|',1x,e9.3)))
    630 format( //,'  Format Number ',i4)
    631 format(//,'  Format Number ',i4)
    640 format("Table form of A"/(2F8.2))
    650 FORMAT( 5X, 'ng= ', I4, /                                      &
                5X, 'mat_opt= ', I2, /                                 &
                5X, 'src_opt= ', I2, /                                 &
                5X, 'scatp= ', I2 )
    660 format('  Format Number ',0PF17.8,' Ry' )
    670 format(/'xx')
    680 format(/"xx")
    690 format(/ "xx")
    700 format(/ 'xx')
    710 format(/ i5, 'x')
    720 format(// i5, 'x')
    730 format(//)
integer :: z
    x = 5
    740 FORMAT(/1X'(',I2,')', X, A)
end program
