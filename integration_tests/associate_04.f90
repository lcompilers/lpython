PROGRAM ASSOCIATE_04

    REAL :: MYREAL, X, Y, THETA, A
    X = 0.42
    Y = 0.35
    MYREAL = 9.1
    THETA = 1.5
    A = 0.4

    ASSOCIATE ( Z => EXP(-(X**2+Y**2)) * COS(THETA), V => MYREAL)
      PRINT *, A+Z, A-Z, V
      V = V * 4.6
    END ASSOCIATE

    PRINT *, MYREAL

  END PROGRAM ASSOCIATE_04