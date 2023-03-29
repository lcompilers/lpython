from lpython import i32, f64, c64

def set_tuple(a: i32, b: f64) -> tuple[i32, f64, str, c64]:
    t: tuple[i32, f64, str, c64]
    t = (a, b, "", complex(a, b))
    return t

def merge_tuple(a: tuple[i32, f64, str, c64], b: tuple[i32, f64, str, c64]) -> tuple[i32, f64, str, c64]:
    c: tuple[i32, f64, str, c64]
    c = (a[0] + b[0], a[1] + b[1], str(a[0]) + str(b[0]), a[3] + b[3])
    return c

def f():
    i: i32
    j: f64
    t1: tuple[i32, f64, str, c64]
    t2: tuple[i32, f64, str, c64]
    t1 = set_tuple(0, 0.0)
    for i in range(11):
        j = float(i)
        t2 = set_tuple(i, j)
        t1 = merge_tuple(t1, t2)
    assert t1[0] == 55
    assert t1[1] == 55.0
    assert t1[2] == "4510"
    assert t1[3] == complex(55, 55)
    print(t1[0], t1[1], t1[2], t1[3])


def g_check(x: tuple[i32, i32], y: tuple[i32, i32]) -> bool:
    return x[0] == y[0]

def test_issue_1348():
    a11: tuple[i32, i32]
    b11: tuple[i32, i32]
    a11 = (1, 2)
    b11 = (1, 2)
    assert g_check(a11, b11)


def check():
    f()
    test_issue_1348()


check()