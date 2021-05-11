module callback_02

contains

subroutine cb(res, a, b, f)
real, intent(in) :: a, b
real :: res
interface
    subroutine f(x, res)
    implicit none
    real, intent(in) :: x
    real :: res
    end subroutine
end interface
call f(a, res)
print *, res
call f(b, res)
print *, res
res = (b-a)*res
print *, res
end subroutine


real function foo(c, d, res)
real :: c, d, res
call cb(res, c, d, f)
foo = res

contains

subroutine f(x, res)
real, intent(in) :: x
real :: res
res = 2*x
end subroutine f

end function foo

end module

program main
  use callback_02
  real :: res = 0
  res = foo(1.5, 2.0, res)
end program
