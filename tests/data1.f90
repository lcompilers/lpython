program data1
    type person
        integer :: age
        character(20) :: fullname
    end type
    character (len = 10) myname
    integer, dimension (0:9) :: miles
    real, dimension (100, 100) :: skew
    type (person) yourname
    data myname / 'xyz' /, miles / 10 * 0 /
    data ((skew (k, j), j = 1, k), integer(4) :: k = 1, 100) / 5050 * 0.0 /
    data ((skew (k, j), j = k + 1, 100), k = 1, 99) / 4950 * 1.0 /
    data yourname % age, yourname % fullname / 35, 'abc' /
    DATA s/ 1.d0, -1.d0,  0.d0,  0.d0,  0.d0, -1.d0, sin3,  cos3, 0.d0, -1.d0 /

    print *, "Your name is: ", yourname % fullname
    print *, "Your age is: ", yourname % age
    print *, "My name is: ", myname
end program
