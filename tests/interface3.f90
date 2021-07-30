module interface3
implicit none
!checks the syntax
public :: x, y, z, assignment(=), operator(+), operator(.and.), operator(.in.)
public :: operator(*)
public :: operator(/)
public :: operator(/ )
public :: operator(// )

interface
    module procedure sample
end interface

interface A
    module procedure :: sample
end interface A

INTERFACE ASSIGNMENT ( = )
    SUBROUTINE LOGICAL_TO_NUMERIC (N, B)
    INTEGER, INTENT (OUT) :: N
    LOGICAL, INTENT (IN) :: B
    END SUBROUTINE LOGICAL_TO_NUMERIC
END INTERFACE ASSIGNMENT ( = )

interface operator (+)
    module procedure union
end interface operator (+)
interface operator (-)
    module procedure difference
end interface operator (-)
interface operator (*)
    module procedure intersection
end interface operator (*)
interface operator ( / )
end interface operator ( / )
interface operator (/)
end interface operator (/)
interface operator (**)
end interface operator (**)
interface operator (==)
end interface operator (==)
interface operator (/=)
end interface operator (/=)
interface operator (>)
end interface operator (>)
interface operator (>=)
end interface operator (>=)
interface operator (<)
end interface operator (<)
interface operator (<=)
    module procedure subset
end interface operator (<=)
interface operator (.not.)
end interface operator (.not.)
interface operator (.and.)
end interface operator (.and.)
interface operator (.or.)
end interface operator (.or.)
interface operator (.eqv.)
end interface operator (.eqv.)
interface operator (.neqv.)
end interface operator (.neqv.)

abstract interface
end interface

public :: operator(//)

interface operator (//)
end interface  operator (//)

interface write(formatted)
    module procedure :: write_formatted
end interface

interface write(unformatted)
    module procedure :: write_unformatted
end interface

interface read(formatted)
    module procedure :: read_formatted
end interface

interface read(unformatted)
    module procedure :: read_unformatted
end interface

contains

    function f(operator)
    ! Currently parsed as an operator, but AST -> ASR phase can fix that:
    real, intent(in) :: operator (*)
    end function f


end module
