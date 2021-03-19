program associate_02
    implicit none

    integer, pointer :: p1
    real(8), pointer :: p2
    integer, target :: t1 = 2
    real(8), target :: t2 = 2


    p1=>t1
    p2=>t2
    p1 = 1
    p2 = 4.0_4

    print *, p1
    print *, t1

    p1 = p1 + p2

    print *, p1
    print *, t1

    t1 = 8

    print *, p1
    print *, t1
    
end program