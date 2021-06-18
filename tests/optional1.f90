MODULE optmod 
IMPLICIT NONE
CONTAINS
    SUBROUTINE optional_argument(name) 
    CHARACTER(LEN=10), INTENT(IN), OPTIONAL :: name
    IF (PRESENT(name)) THEN
        PRINT*, 'Hello '// name
    ELSE
        PRINT*, 'Hello world!'
        END IF
    END SUBROUTINE optional_argument 
END MODULE optmod
