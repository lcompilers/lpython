module class_circle2
  implicit none
  private
  real :: pi = 3.1415926535897931d0 ! Class-wide private constant

  type, public :: Circle
     real :: radius
   contains
     procedure :: circle_area  !=> circle_area ! Can you point to implementation in diff module?
     procedure :: circle_print
  end type Circle
contains
  real function circle_area(this)
    class(Circle), intent(in) :: this
    circle_area = pi * this%radius**2
  end function circle_area

  subroutine circle_print(this)
    class(Circle), intent(in) :: this
    real :: area
    area = this%circle_area()  ! Call the type-bound function
    print *, 'Circle: r = ', this%radius, ' area = ', area
  end subroutine circle_print
end module class_Circle2


program circle_test
  use class_circle2, only: Circle
  implicit none

  type(Circle) :: c     ! Declare a variable of type Circle.
  !c = Circle(1.5)       ! Use the implicit constructor, radius = 1.5.
  !c = Circle       ! Works in gfortran, but not ifort or flang
  c%radius = 1.5
  call c%circle_print          ! Call the type-bound subroutine
end program circle_test
