from ltypes import i32, f64

def f():
    t1: tuple[i32, f64, str, bool]
    t1 = (1, 2.0, "3", True)
    assert t1[0] == 1
    assert t1[2] == "3"
    assert t1[3]

    t2: tuple[i32, i32]
    t2 = (1, 2)
    a: i32
    b: i32
    (a, b) = t2
    assert a == 1
    assert b == 2

f()
