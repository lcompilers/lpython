program format1
implicit none
    1 format(/,'BDF2 step ',i6,': T=',es12.5,', H=',es12.5,', ETAH=',es12.5)
    2 format(/ ,'BDF2 step ',i6,': T=',es12.5,', H=',es12.5,', ETAH=',es12.5)
    3 format( /,'BDF2 step ',i6,': T=',es12.5,', H=',es12.5,', ETAH=',es12.5)
    4 format(2x,'Preconditioner update FAILED at T=',es12.5,', ETAH=',es12.5)
end program
