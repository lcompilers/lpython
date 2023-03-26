from lpython import i32, overload

@overload
def foo(a: i32, b: i32) -> i32:
    return a*b

@overload
def foo(a: i32) -> i32:
    return a**2

@overload
def foo(a: str) -> str:
    return "lpython-" + a

@overload
def test(a: i32) -> i32:
    return a + 10

@overload
def test(a: bool) -> i32:
    if a:
        return 10
    return -10
