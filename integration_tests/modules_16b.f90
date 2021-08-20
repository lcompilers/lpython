module modules_16b

contains

integer function sfloor(x) result(r)
real, intent(in) :: x
r = x
end function

integer function imodulo(x) result(r)
integer, intent(in) :: x
r = sfloor(real(x))
end function

end module
