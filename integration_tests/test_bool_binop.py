from ltypes import i32, f64

def f():
    i: i32
    i = i32(True + True)
    assert i == 2

    i = i32(True - False)
    assert i == 1

    i = i32(False * True)
    assert i == 0

    i = i32(True // True)
    assert i == 1

    i = i32(False // True)
    assert i == 0

    i = i32(True ** True)
    assert i == 1

    f: f64
    b1: bool = False
    b2: bool = True
    f = f64(b1/b2)
    assert f == 0.0

f()
