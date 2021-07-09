module class_Circle
  implicit none
  private
  real :: pi = 3.1415926535897931d0 ! Class-wide private constant

  type, public :: Circle
     real :: radius
   contains
     procedure :: area => circle_area ! Can you point to implementation in diff module?
     procedure :: print => circle_print
  end type Circle
contains
  function circle_area(this) result(area)
    class(Circle), intent(in) :: this
    real :: area
    area = pi * this%radius**2
  end function circle_area

  subroutine circle_print(this)
    class(Circle), intent(in) :: this
    real :: area
    area = this%area()  ! Call the type-bound function
    print *, 'Circle: r = ', this%radius, ' area = ', area
  end subroutine circle_print
end module class_Circle


program circle_test
  use class_Circle, only: Circle
  implicit none

  type(Circle) :: c     ! Declare a variable of type Circle.
  !c = Circle(1.5)       ! Use the implicit constructor, radius = 1.5.
  !c = Circle       ! Works in gfortran, but not ifort or flang
  c%radius = 1.5
  call c%print          ! Call the type-bound subroutine
end program circle_test
