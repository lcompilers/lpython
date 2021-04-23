program associate_02
    implicit none

    integer, pointer :: p1
    real(8), pointer :: p2
    integer, target :: t1 = 2
    real(8), target :: t2 = 2.0_8
    complex(4), pointer :: p3
    complex(4), target :: t3 = (2.0_4, 3.0_4)

    p1=>t1
    p2=>t2
    p3=>t3
    p1 = 1
    p2 = 4.0_4

    print *, p1
    print *, t1

    t1 = p2 + p1

    print *, p1
    print *, t1

    t1 = 8

    print *, p1
    print *, t1

    p3 = 2*p3
    print *, p3
    print *, t3
    
end program