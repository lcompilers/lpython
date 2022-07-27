from ltypes import i32, f64

def f():
    t1: tuple[i32, f64, str, bool]
    t2: tuple[i32, f64, str, bool]
    t1 = (1, 2.0, "3", True)
    t2 = (2, 3.0, "3", False)
    assert t2[0] - t1[0] == 1
    assert abs(t2[1] - t1[1] - 1.0) <= 1e-12
    assert t1[2] == t2[2]
    assert t1[3] or t2[3]

    t3: tuple[i32, i32]
    t4: tuple[f64, f64]
    t3 = (1, 2)
    t4 = (1.0, 2.0)
    a: i32
    b: i32
    af: f64
    bf: f64
    (a, b) = t3
    (af, bf) = t4
    assert af == a
    assert bf == b

f()
