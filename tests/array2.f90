program array2
implicit none

type :: der_type
    real :: r
    integer :: i
end type

! real, dimension(5) :: a, b
! complex, dimension(6) :: x
! integer, dimension(3) :: c
! logical, dimension(2) :: d
type(der_type), dimension(7) :: p

! real, dimension(2,3) :: e
! complex, dimension(6,8) :: y
! integer, dimension(3,4) :: f
! logical, dimension(5,2) :: g

! real, dimension(2:3,3:4,4:5) :: h
! complex, dimension(6:7,8:10,11:13) :: z
! integer, dimension(3:4,4:5,3:4) :: i
! logical, dimension(5:6,2:3,2:4) :: j

end program
