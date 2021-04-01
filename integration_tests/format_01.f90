program format1
implicit none
    1 format(/,'BDF2 step ',i6,': T=',es12.5,', H=',es12.5,', ETAH=',es12.5)
    2 format(/ ,'BDF2 step ',i6,': T=',es12.5,', H=',es12.5,', ETAH=',es12.5)
    3 format( /,'BDF2 step ',i6,': T=',es12.5,', H=',es12.5,', ETAH=',es12.5)
    4 format(2x,'Preconditioner update FAILED at T=',es12.5,', ETAH=',es12.5)
    5 format(/)
    6 format(/ )
    7 format( /)
    8 format(i6, /)
    9 format(i6,/)
    10 format(/ , /)
    422 FORMAT( /, 4X, 'Manufactured/Computed Solutions Max Diff=',    &
                 ES13.6 )
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
end program
