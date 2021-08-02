c Test comments and line continuation
      program fixed_form2
c Comment 1
C Comment 2
! Comment col 1
 ! Comment col 2
  ! Comment col 3
   ! Comment col 4
    ! Comment col 5
      ! Comment col 7
       ! Comment col 8
                               ! Comment 3
      integer :: a ! Comment 4
      print *, "OK1"
     !, "OK2" ! The first ! is not a comment, but line continuation
      print *, "OK1"
!    !, "F2" ! This whole line is a comment
c    !, "F3" ! This whole line is a comment
C    !, "F4" ! This whole line is a comment
*    !, "F5" ! This whole line is a comment
 !   !, "F6" ! This whole line is a comment
  !  !, "F7" ! This whole line is a comment
   ! !, "F8" ! This whole line is a comment
    !!, "F9" ! This whole line is a comment
     ;, "OK2" ! line continuation to the previous non-comment line
     1, "OK3" ! line continuation
     $, "OK4" ! line continuation
     0print *, "1" ! not line continuation, new statement
      print *, "2" ! not line continuation, new statement
      print *, "1", "1   ! not comment
     $  finish string"
      print *, "1", "1 "" ' 2 """" 3
     $ ! also not comment
     $ finish string"
      print *, "1", '1 '' " 2 '''' 3
     $ ! also not comment
     $ finish string'
      end ! Comment 5
! Comment
