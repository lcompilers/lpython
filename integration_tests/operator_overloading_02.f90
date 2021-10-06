module overload_assignment_m
    implicit none
    private
    public assignment (=)

    interface assignment (=)
        module procedure logical_gets_integer
    end interface
contains
    subroutine logical_gets_integer(tf, i)
        logical, intent (out) :: tf
        integer, intent (in)  :: i

        tf = (i == 0)
    end subroutine
end module

program main
    use overload_assignment_m
    implicit none
    logical :: tf

    tf = 0
    print *, "tf=0:", tf  ! Yields: T
    tf = 1
    print *, "tf=1:", tf  ! Yields: F
end program