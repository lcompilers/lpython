program program2
implicit none
call b
call b()

contains

    subroutine b()
    print *, "b"
    end subroutine

end program
