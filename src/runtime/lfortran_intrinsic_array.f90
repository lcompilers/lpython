module lfortran_intrinsic_array
implicit none

interface
    integer function size(x) result(r)
    integer, intent(in) :: x(:)
    end function
end interface

interface
    integer function lbound(x, dim) result(r)
    integer, intent(in) :: x(:)
    integer, intent(in) :: dim
    end function
end interface

interface
    integer function ubound(x, dim) result(r)
    integer, intent(in) :: x(:)
    integer, intent(in) :: dim
    end function
end interface

end module
