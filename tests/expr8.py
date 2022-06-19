def test_binop():
    x: i32
    x2: f32
    x = 2**3
    x2 = 2**3.5
    x = 54 - 100
    x2 = (3.454 - 765.43) + 534.6
    x2 = 5346.565 * 3.45
    x2 = 5346.565 ** 3.45

    # binop on logical type
    x = True + True
    x = True - False
    x = True * False
    x = True ** False
    b1: bool
    b2: bool
    b1 = True
    b2 = False
    x = b1//b1
    x = b1**b2

test_binop()
