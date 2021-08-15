program data1
    type person
        integer :: age
        character(20) :: fullname
    end type
    character (len = 10) myname
    integer, dimension (0:9) :: miles
    data myname / 'xyz' /, miles / 10 * 0 /
    real, dimension (100, 100) :: skew
    type (person) yourname
    data yourname % age, yourname % fullname / 35, 'abc' /
    data ((skew (k, j), j = 1, k), integer(4) :: k = 1, 100) / 5050 * 0.0 /
    data ((skew (k, j), j = 1, k, k), integer(4) :: k = 1, 100, 2) / 50 * 0.0 /
    data ((skew (k, j), j = k + 1, 100), k = 1, 99) / 4950 * 1.0 /
    data ((skew (k, j), j = k + 1, 100, k), k = 1, 99, 1) / 473 * 1.0 /
    real(8), parameter :: sin3 = sin(3.d0)
    real(8), parameter :: cos3 = cos(3.d0)
    real(8) s(10)
    DATA s/ 1.d0, -1.d0,  0.d0,  0.d0,  0.d0, -1.d0, sin3,  cos3, 0.d0, -1.d0 /
    integer :: iarx(3,1), iary(3,1)

    print *, "Your name is: ", yourname % fullname
    print *, "Your age is: ", yourname % age
    data(iarx(i,1), iary(i,1),i=1,3)/  1, 9, 1950,1350, 4350, 4/
    print *, "My name is: ", myname
    data(iary(i,1),i=1,3)/  1, 9, 1950 /
end program
