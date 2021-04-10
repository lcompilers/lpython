PROGRAM Block_02
    INTEGER :: A
    A = 10
    BLOCK
        INTEGER :: B
        B = A + 5
        CALL Square(B)
    END BLOCK
END PROGRAM Block_02

SUBROUTINE Square(B)
    INTEGER :: B, Result
    Result = B * B
    PRINT *, Result
END SUBROUTINE Square
