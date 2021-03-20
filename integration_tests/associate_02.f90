program associate_02
    implicit none

    real, pointer :: p1
    ! real(8), pointer :: p2
    real, target :: t1 = 2.0
    ! real(8), target :: t2 = 2


    p1=>t1
    ! p2=>t2
    ! p1 = 1
    ! p2 = 4.0_4

    ! print *, p1
    ! print *, t1

    t1 = 1.0 + p1

    ! print *, p1
    print *, t1

    ! t1 = 8

    ! print *, p1
    ! print *, t1
    
end program