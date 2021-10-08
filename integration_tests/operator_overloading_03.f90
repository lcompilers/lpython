module operator_overloading_01_overload_comp_m
    implicit none
    public operator (>)

    interface operator (>)
        module procedure greater_than_inverse
    end interface

    interface operator (<)
        module procedure less_than_inverse
    end interface
contains
    pure logical function greater_than_inverse(log1, log2)
        logical, intent (in) :: log1, log2

        if( log1 .eqv. .true. .and. log2 .eqv. .false. ) then
            greater_than_inverse = .true.
        else
            greater_than_inverse = .false.
        end if
    end function

    pure logical function less_than_inverse(log1, log2)
        logical, intent (in) :: log1, log2

        if( log1 .eqv. .false. .and. log2 .eqv. .true. ) then
            less_than_inverse = .true.
        else
            less_than_inverse = .false.
        end if
    end function
end module

program operator_overloading_01
  use operator_overloading_01_overload_comp_m
  implicit none
  logical, parameter :: T = .true., F = .false.

  print *, "T>T:", T>T  ! Yields: F
  print *, "T>F:", T>F  ! Yields: F
  print *, "F>T:", F>T  ! Yields: T
  print *, "F>F:", F>F  ! Yields: F
  print *, "T<T:", T<T  ! Yields: F
  print *, "T<F:", T<F  ! Yields: T
  print *, "F<T:", F<T  ! Yields: F
  print *, "F<F:", F<F  ! Yields: F
end program
