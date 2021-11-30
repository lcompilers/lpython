module subroutines_07_mod

    interface f_int
        procedure f
    end interface

contains

    subroutine f(a, b, c, d, kind)
        integer, intent(in) :: a, d, c
        integer, intent(in), optional :: kind
        integer, intent(out) :: b
        b = a + 1 + c + d
    end subroutine

end module

program subroutines_07
    use subroutines_07_mod
    implicit none
    integer :: i, j, k, l
    i = 1
    j = 1
    k = 1
    l = 1
    call f_int(i, j, k, l)
    if (j /= 4) error stop

end program