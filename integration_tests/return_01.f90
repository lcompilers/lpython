program main
    integer main_out
    main_out = main1()    
    print *, "main1 called"
    contains
    integer function main1()
        integer :: i = 10
        if (i .GT. 5) then
            main1 = i
            print *, "early return"
            return  
        end if
        print *, "normal return"
        main1 = i
    end function main1
end program
