from lpython import i32, Callable


def f(x: i32) -> i32:
    return x + 1

def f2(x: i32) -> i32:
    return x + 10

def f3(x: i32) -> i32:
    return f(x) + f2(x)


def g(func: Callable[[i32], i32], arg: i32) -> i32:
    ret: i32
    ret = func(arg)
    return ret


def check():
    assert g(f, 10) == 11
    assert g(f2, 20) == 30
    assert g(f3, 5) == 21


check()
