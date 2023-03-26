from lpython import i32, f32, c32

def test_UnaryOp():
    a: i32
    a = +4
    a = -500
    a = ~5
    b: bool
    b = not 5
    b = not -1
    b = not 0

    f: f32
    f = +f32(1.0)
    f = -f32(183745.534)

    b1: bool
    b2: bool
    b3: bool
    b1 = True
    b2 = not False
    b3 = not b2
    a = +True
    a = -False
    a = ~True

    c: c32
    c = +c32(complex(1, 2))
    c = -c32(complex(3, 65.0))
    b1 = not complex(3, 4)
    b2 = not complex(0, 0)
