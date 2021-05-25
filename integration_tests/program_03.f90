program closuretest
    ! Obtained from https://events.prace-ri.eu/event/694/attachments/627/918/advanced_fortran_handout.pdf

    integer :: z
    do z=1,10
        print *, apply(add_z, 1)
    end do
    
    contains
    
    function apply(fun,x) result(y)
            
    !procedure(intfun) :: fun
    integer :: x, y

    interface
        integer function fun(x)
        integer :: x
        end function fun
    end interface
    y = fun(x)
    end function
    
    function add_z(x) result(y)
    integer :: x, y
    y = x + z
    end function

end program 
