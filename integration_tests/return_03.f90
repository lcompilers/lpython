program main
    integer :: main_out = 999
    call main1(main_out)
    print *, "main1 called"
    contains
    subroutine main1(out_var)
        integer :: out_var
        integer :: i = 10
        if (i .GT. 5) then
            out_var = i
            print *, "early return"
            return  
        end if
        print *, "normal return"
        out_var = i
    end subroutine main1
end program
