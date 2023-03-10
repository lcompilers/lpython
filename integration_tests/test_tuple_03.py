from ltypes import i32, f64

def f():
    t1: tuple[i32, f64, str] = (1, 2.0, "3")

    assert t1[-1] == "3"
    assert t1[-2] == 2.0
    assert t1[-3] == 1

f()