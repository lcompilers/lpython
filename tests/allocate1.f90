program hello
implicit none
    character(:), allocatable :: cmd
    integer :: cmdlen
    call get_command(length=cmdlen)

    if (cmdlen>0) then
        allocate(character(cmdlen) :: cmd)
        call get_command(cmd)
        print *, "hello ", cmd
    end if
end program
