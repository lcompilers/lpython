module interface_08
    abstract interface
        subroutine sub(x,y)
            integer, intent(in) :: x
            integer, intent(in) :: y
        end subroutine
    end interface
end module