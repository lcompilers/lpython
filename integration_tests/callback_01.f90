module callback_01

contains

real function cb(f, a, b)
real, intent(in) :: a, b
interface
    real function f(x)
    implicit none
    real, intent(in) :: x
    end function
end interface
cb = (b-a) + f(a) + f(b)
end function


subroutine foo(c, d)
real :: c, d
print *, cb(f, c, d)

contains

real function f(x)
real, intent(in) :: x
f = 2*x
end function f

end subroutine foo

end module

program main
  use callback_01
  call foo(1.5, 2.0)
end program
