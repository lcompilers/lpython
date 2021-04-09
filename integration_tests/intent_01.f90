module dflt_intent

contains

subroutine foo(c, d)
real :: c, d, e, g
e = f(c)
g = f(d)

contains

real function f(x)
real, intent(in) :: x
f = 2*x
print *, f
end function f

end subroutine foo

end module

program main
  use dflt_intent
  call foo(0.0, 2.0)
end program
