c Test comments
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
!    !, "OK2" ! But this whole line is a comment
      end ! Comment 5
! Comment
