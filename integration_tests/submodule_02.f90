! Reference - http://fortranwiki.org/fortran/show/Submodules
module stdlib_kinds
use iso_fortran_env, only: rkind1 => real32
use iso_fortran_env, only: int32
implicit none
public rkind1, int32
end module stdlib_kinds

module points
  use :: stdlib_kinds, only: rkind  => rkind1
  implicit none

  type :: point
     real :: x, y
  end type point

  interface
     module function point_dist(a, b) result(distance)
       type(point), intent(in) :: a, b
       integer(rkind) :: distance
     end function point_dist
  end interface
end module points

submodule (points) points_a
contains
  module function point_dist(a, b) result(distance)
    type(point), intent(in) :: a, b
    integer(rkind) :: distance
    distance = sqrt((a%x - b%x)**2 + (a%y - b%y)**2)
  end function point_dist
end submodule points_a

program submodules_02
implicit none
end program