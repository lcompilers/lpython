module interface3
implicit none
!checks the syntax
public :: x, y, z, assignment(=), operator(+), operator(.and.), operator(.in.)
public :: operator(*)

interface
    module procedure sample
end interface

interface A
    module procedure :: sample
end interface

INTERFACE ASSIGNMENT ( = )
    SUBROUTINE LOGICAL_TO_NUMERIC (N, B)
    INTEGER, INTENT (OUT) :: N
    LOGICAL, INTENT (IN) :: B
    END SUBROUTINE LOGICAL_TO_NUMERIC
END INTERFACE

interface operator (+)
    module procedure union
end interface
interface operator (-)
    module procedure difference
end interface
interface operator (*)
    module procedure intersection
end interface
interface operator ( / )
end interface
interface operator (**)
end interface
interface operator (==)
end interface
interface operator (/=)
end interface
interface operator (>)
end interface
interface operator (>=)
end interface
interface operator (<)
end interface
interface operator (<=)
    module procedure subset
end interface
interface operator (.not.)
end interface
interface operator (.and.)
end interface
interface operator (.or.)
end interface
interface operator (.eqv.)
end interface
interface operator (.neqv.)
end interface

abstract interface
end interface

public :: operator(//)

interface operator (//)
end interface

contains

    function f(operator)
    ! Currently parsed as an operator, but AST -> ASR phase can fix that:
    real, intent(in) :: operator (*)
    end function f


end module
