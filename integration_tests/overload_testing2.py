from lpython import i32, overload

@overload
def foo2(a: i32, b: i32) -> i32:
    return a + b

@overload
def foo2(a: i32) -> i32:
    return a**3

@overload
def foo2(a: str) -> str:
    return "lpython-super-fun-" + a

@overload
def test2(a: i32) -> i32:
    return a + 30

@overload
def test2(a: bool) -> i32:
    if a:
        return 30
    return -30
