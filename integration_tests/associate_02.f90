program pointer_01
    implicit none

    integer, pointer :: p1
    integer, target :: t1 = 2

    p1=>t1
    p1 = 1

    print *, p1
    print *, t1

    p1 = p1 + 4

    print *, p1
    print *, t1

    t1 = 8

    print *, p1
    print *, t1
    
end program