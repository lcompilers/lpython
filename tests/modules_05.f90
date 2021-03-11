module access_vars
implicit none
private
public publ

real :: priv  = 1.5
real :: publ = 2.5

contains

integer function print_vars(a, b)
real, intent(in) :: a
real, intent(in) :: b
print *, "priv = ", a
print *, "publ = ", b
print_vars = 1
end function print_vars

end module access_vars
