program associate_03
    implicit none
    
    integer, pointer :: p1
    integer, target :: t1 = 2, t2 = 1
    integer :: i
    
    print *, t1, t2
    ! Runtime pointer association
    if (t1 > t2) then
        p1 => t1
    else
        p1 => t2
    end if
    print *, p1
    ! Does not work yet:
    !if (p1 == t2) error stop
    i = p1
    if (i == t2) error stop
    
end program
    