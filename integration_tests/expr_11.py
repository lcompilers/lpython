from lpython import i32

def f():
    i: i32
    i = 3
    b: bool
    b = bool(i)
    assert b
    print(b)

f()
