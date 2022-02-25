from ltypes import overload, i32

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


assert foo(2) == 4
assert foo(2, 10) == 20
assert foo("hello") == "lpython-hello"
assert test(10) == 20
assert test(False) == -test(True) and test(True) == 10
