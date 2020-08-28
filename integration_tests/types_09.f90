program types_09
use iso_c_binding, only: c_char
implicit none
integer, parameter :: ucs4=selected_char_kind("ISO_10646")
character(1,ucs4), parameter :: nen=char(int(Z'5e74'),ucs4)

contains

    subroutine f(s, s2)
    character(kind=c_char, len=:), allocatable, intent(in) :: s
    character(len=:), allocatable, intent(in) :: s2
    end subroutine

end program
