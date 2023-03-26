from lpython import i32, Const, inline

@inline
def f(x: i32) -> i32:
    offset: Const[i32] = 1
    return x + offset

def test_f():
    x: i32 = 0
    x = f(x)
    print(x)
    assert x == 1
    x = f(x)
    print(x)
    assert x == 2

test_f()
