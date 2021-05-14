MODULE DOT
! Module for dot product of two real arrays of rank 1.
! The caller needs to ensure that exceptions do not cause halting.
! Source https://j3-fortran.org/doc/year/18/18-007r1.pdf
    USE, INTRINSIC :: IEEE_EXCEPTIONS
    LOGICAL :: MATRIX_ERROR = .FALSE.

    INTERFACE OPERATOR(.dot.)
        MODULE PROCEDURE MULT
    END INTERFACE

CONTAINS

    REAL FUNCTION MULT (A, B)
        REAL, INTENT (IN) :: A(:), B(:)
        INTEGER I
        LOGICAL OVERFLOW

        IF (SIZE(A) /= SIZE(B)) THEN
            MATRIX_ERROR = .TRUE.
            RETURN
        END IF
        ! The processor ensures that IEEE_OVERFLOW is quiet.

        MULT = 0.0
        DO I = 1, SIZE (A)
            MULT = MULT + A(I)*B(I)
        END DO
        CALL IEEE_GET_FLAG (IEEE_OVERFLOW, OVERFLOW)
        IF (OVERFLOW) MATRIX_ERROR = .TRUE.
    END FUNCTION MULT
END MODULE DOT
