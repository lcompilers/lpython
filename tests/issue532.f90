! This is a test for https://gitlab.com/lfortran/lfortran/-/issues/532
module issue532_mod

contains

integer function sfloor(x) result(r)
real, intent(in) :: x
r = x
end function

integer function imodulo(x, y) result(r)
integer, intent(in) :: x, y
r = sfloor(real(x))*y
end function

end module

program issue532
end program
