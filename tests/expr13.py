def test_Compare():
    a: bool
    a = 5 > 4
    a = 5 <= 4
    a = 5 < 4
    a = 5.6 >= 5.59999
    a = 3.3 == 3.3
    a = 3.3 != 3.4
    a = complex(3, 4) == complex(3., 4.)
