! Submodules test, taken from:
! https://www.ibm.com/docs/en/xl-fortran-aix/15.1.3?topic=techniques-submodules-fortran-2008
MODULE m1
  TYPE Base
    INTEGER :: i
  END TYPE

  INTERFACE
    MODULE SUBROUTINE sub1(i, b)   ! Module procedure interface body for sub1
      INTEGER, INTENT(IN) :: i
      TYPE(Base), INTENT(IN) :: b
    END SUBROUTINE
  END INTERFACE
END MODULE

MODULE m2
  USE m1                           ! Use association of module m1

  INTERFACE
    REAL MODULE FUNCTION func1()   ! Module procedure interface body for func1
    END FUNCTION

    MODULE FUNCTION func2(b)       ! Module procedure interface body for func2
      TYPE(Base) :: b
      TYPE(Base) :: func2
    END FUNCTION
  END INTERFACE
END MODULE

MODULE m4
  USE m1                           ! Use association of module m1
  TYPE, EXTENDS(Base) :: NewType
    REAL :: j
  END TYPE
END MODULE

SUBMODULE (m1) m1sub
  USE m4                           ! Use association of module m4

  CONTAINS
    MODULE SUBROUTINE sub1(i, b)   ! Implementation of sub1 declared in m1
      INTEGER, INTENT(IN) :: i
      TYPE(Base), INTENT(IN) :: b
      PRINT *, "sub1", i, b
    END SUBROUTINE
END SUBMODULE

SUBMODULE (m2) m2sub

  CONTAINS
    REAL MODULE FUNCTION func1()   ! Implementation of func1 declared in m2
      func1 = 20
    END FUNCTION
END SUBMODULE

SUBMODULE (m2:m2sub) m2sub2

  CONTAINS
    MODULE FUNCTION func2(b)       ! Implementation of func2 declared in m2
      TYPE(Base) :: b
      TYPE(Base) :: func2
      func2 = b
    END FUNCTION
END SUBMODULE

MODULE m3
  INTERFACE
    SUBROUTINE interfaceSub1(i, b)
      USE m1
      INTEGER, INTENT(IN) :: i
      TYPE(Base), INTENT(IN) :: b
    END SUBROUTINE

    REAL FUNCTION interfaceFunc1()
    END FUNCTION

    FUNCTION interfaceFunc2(b)
      USE m1
      TYPE(Base) :: b
      TYPE(Base) :: interfaceFunc2
    END FUNCTION
  END INTERFACE

  TYPE Container
    PROCEDURE(interfaceSub1), NOPASS, POINTER :: pp1
    PROCEDURE(interfaceFunc1), NOPASS, POINTER :: pp2
    PROCEDURE(interfaceFunc2), NOPASS, POINTER :: pp3
  END TYPE
END MODULE

PROGRAM example
  USE m1
  USE m2
  USE m3
  TYPE(Container) :: c1
  c1%pp1 => sub1
  c1%pp2 => func1
  c1%pp3 => func2

  CALL c1%pp1(10, Base(11))
  PRINT *, "func1", int(c1%pp2())
  PRINT *, "func2", c1%pp3(Base(5))
END PROGRAM
