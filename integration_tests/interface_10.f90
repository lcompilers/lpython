module interface_10_m
    interface chars
        module procedure identity
    end interface

contains

    subroutine identity(x, y)
        character, intent(in) :: x
        character, intent(out) :: y
        y = x
    end subroutine
end module

program interface_10
    character :: x, y
    x = 'A'
    call chars(x, y)
    print *, x
end program