program dim_assgn_test
! Warning:
integer, private, dimension(2,2) :: a(2,3)
! Error:
x = 5
end program
