module operator_overloading_01_overload_asterisk_m
    implicit none
    public operator (*)

    interface operator (*)
        module procedure logical_and
    end interface

    interface operator (+)
        module procedure bin_add
    end interface
contains
    pure logical function logical_and(log1, log2)
        logical, intent (in) :: log1, log2

        logical_and = (log1 .and. log2)
    end function

    pure integer function bin_add(log1, log2)
        logical, intent (in) :: log1, log2

        if( log1 .and. log2 ) then
            bin_add = 2
        elseif( .not. log1 .and. .not. log2 ) then
            bin_add = 0
        else
            bin_add = 1
        endif
    end function
end module

program operator_overloading_01
  use operator_overloading_01_overload_asterisk_m
  implicit none
  logical, parameter :: T = .true., F = .false.

  print *, "T*T:", T*T  ! Yields: T
  print *, "T*F:", T*F  ! Yields: F
  print *, "F*T:", F*T  ! Yields: F
  print *, "F*F:", F*F  ! Yields: F
  print *, "T+T:", T+T  ! Yields: 2
  print *, "T+F:", T+F  ! Yields: 1
  print *, "F+T:", F+T  ! Yields: 1
  print *, "F+F:", F+F  ! Yields: 0
end program
