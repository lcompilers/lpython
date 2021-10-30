program intrinsics_23
implicit none
! integer, parameter :: x = huge(0)
! real(kind=4), parameter :: y = huge(0.0)
! real(kind=8), parameter :: z = huge(0.0d0)
!
! if( x /= 2147483647 )               error stop
! if( y /= 3.40282347E+38 )           error stop
! if( z /= 1.7976931348623157E+308 )  error stop

print *, huge(0),huge(0.0), huge(0.0d0)
end program
