from lpython import i32

def f():
    x: i32 = 2
    y: i32 = 1
    z: i32 = 1
    t: i32 = 1
    assert x > y == z
    assert not (x == y == z)
    assert y == z == t != x
    assert x > y == z >= t
    t = 0
    assert x > y == z >= t
    t = 4
    assert not (x > y == z >= t)
    assert t > x > y == z
    assert 3 > 2 >= 0 <= 6
    assert t > y < x
    assert not (2 == 3 > 4)


f()
