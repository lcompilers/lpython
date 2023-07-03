from lpython import i32

def f():
    i: i32
    i = 5
    res: i32
    res = ~i
    assert res == -6

    i = -235346
    assert ~i == 235345

f()
