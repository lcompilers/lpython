module lfortran_intrinsic_array
implicit none

interface
    integer function size(x) result(r)
    integer, intent(in) :: x(:)
    end function

    integer function lbound(x, dim) result(r)
    integer, intent(in) :: x(:)
    integer, intent(in) :: dim
    end function

    integer function ubound(x, dim) result(r)
    integer, intent(in) :: x(:)
    integer, intent(in) :: dim
    end function

    integer function max(a, b) result(r)
    integer, intent(in) :: a, b
    end function

    integer function min(a, b) result(r)
    integer, intent(in) :: a, b
    end function
end interface

end module
