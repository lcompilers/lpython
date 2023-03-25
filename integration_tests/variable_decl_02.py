from lpython import i32, i64

def f():
    d: i32 = 0
    c: i64 = i64(d + 1)
    b: i64 = i64(d) + c + i64(2)
    a: i32 = i32(b) + 20
    print(a, b, c, d)
    assert d == 0
    assert c == i64(1)
    assert b == i64(3)
    assert a == 23

f()
