PROGRAM Block_02
    INTEGER :: A
    A = 10
    1 loop: BLOCK
        INTEGER :: B
        A = A + 5
        IF (A.EQ.15) GO TO 1
    2   B = A / 2
        IF(B.EQ.10) GO TO 2
    END BLOCK loop
    CALL Square(B)
END PROGRAM Block_02

SUBROUTINE Square(B)
    INTEGER :: B, Result
    Result = B * B
    PRINT *, Result
END SUBROUTINE Square
