def test_UnaryOp():
    a: i32
    a = +4
    a = -500
    a = ~5
    a = not 5
    a = not -1
    a = not 0

    f: f32
    f = +1.0
    f = -183745.534

    b1: bool
    b2: bool
    b3: bool
    b1 = True
    b2 = not False
    b3 = not b2
