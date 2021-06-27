program goto2
implicit none
      integer :: A
      A = 0

    1 continue
      write(*,*)
      A = A + 1

      if (A.GT.2) go to 95

      write(*,*) 'A =', A,'  Starting'
      if (A.EQ.1) go to 10
      go to 20

   10 continue
      write(*,*) 'A =',A,'  Got to 10'
      goto 30

   20 continue
      write(*,*) 'A =',A,'  Got to 20'

   30 continue
      write(*,*) 'A =',A,'  Got to 30'
      if (A.EQ.2) go to 99
      goto 1


   95 continue
      write(*,*) 'A =',A,'  Got to 95'


   99 continue
      write(*,*) 'A =',A,'  Got to 99'

      stop ' '
end
