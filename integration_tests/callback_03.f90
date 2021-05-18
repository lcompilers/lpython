module callback_03

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


subroutine foo1(c, d)
real :: c, d
print *, cb(f, c, d)

contains

real function f(x)
real, intent(in) :: x
f = 2*x
end function f

end subroutine foo1


subroutine foo2(c, d)
real :: c, d
print *, cb(f, c, d)

contains

real function f(x)
real, intent(in) :: x
f = -2*x
end function f

end subroutine foo2

end module

program main
  use callback_03
  call foo1(1.5, 2.0)
  call foo2(1.5, 2.0)
end program main
