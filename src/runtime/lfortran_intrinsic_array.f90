module lfortran_intrinsic_array
implicit none

interface
    integer function size(x) result(r)
    integer, intent(in) :: x(:)
    end function
end interface

end module
