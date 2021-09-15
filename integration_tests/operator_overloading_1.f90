module overload_asterisk_m
    implicit none
    public operator (*)

    interface operator (*)
        module procedure logical_and
    end interface
contains
    pure logical function logical_and(log1, log2)
        logical, intent (in) :: log1, log2

        logical_and = (log1 .and. log2)
    end function
end module

program main
  use overload_asterisk_m
  implicit none
  logical, parameter :: T = .true., F = .false.

  print *, "T*T:", T*T  ! Yields: T
  print *, "T*F:", T*F  ! Yields: F
  print *, "F*T:", F*T  ! Yields: F
  print *, "F*F:", F*F  ! Yields: F
end program