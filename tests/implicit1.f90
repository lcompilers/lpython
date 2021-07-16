program implicit1
! AST only
implicit none
IMPLICIT NONE
implicit none (external)
implicit none (type)
implicit none (external, type)
implicit none (type, external)

implicit real (a-h,o-z)
implicit real (dp) (a-h,o-z)
implicit real*8 (a-h,o-z)
implicit real(8) (a-h,o-z)

implicit double precision (a-h,o-z)

implicit character (c,o-z)
implicit character (id) (a-z)

implicit integer (i-n)
implicit integer (i,j,k,l,m,n)
implicit integer (i,j-l,m,n)
implicit integer (dp) (a-h,o-z)
implicit integer*8 (i,j-l,m,n)
IMPLICIT INTEGER (A, C)
IMPLICIT INTEGER*4 (C, D-x)
IMPLICIT INTEGER(4) (C, D-x)

implicit logical (l, u-z)
implicit logical (dp) (a-h,o-z)
implicit logical*4 (l, u-z)
implicit logical(4) (l, u-z)

implicit complex (z)
implicit complex (dp) (a-h,o-z)
IMPLICIT COMPLEX (C)
implicit complex*4 (z)
implicit complex(4) (z)

IMPLICIT TYPE(BLOB) (A)
IMPLICIT class(X) (A-b)
IMPLICIT procedure(Y) (A-b)
end program
