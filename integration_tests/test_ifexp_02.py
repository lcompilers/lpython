from lpython import i32

def g() -> i32:
    return 10

def f():
    i: i32
    j: i32
    k: i32
    i = 5
    j = 6
    k = g() if i > j else g() - 1

    print(k)
    assert k == 9

f()
