def test_Compare():
    a: bool
    a = 5 > 4
    a = 5 <= 4
    a = 5 < 4
    a = 5.6 >= 5.59999
    a = 3.3 == 3.3
    a = 3.3 != 3.4
    a = complex(3, 4) == complex(3., 4.)

    # string comparison
    a = "abc" > "abd"
    a = "" < "s"
    a = "-abs" >= "abs"
    a = "abcd" <= "abcde"
    a = "abc" == "abc"
    a = "abc" != "abd"
    a = "" == "+"

    a = True > False
    a = True == True
    a = False != True
    a = False >= True
