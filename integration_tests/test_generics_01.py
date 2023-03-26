from overload_testing import foo, test
from lpython import overload, i32, i64
import overload_testing2


@overload
def foo1(a: i32, b: i32) -> i32:
    return a-b

@overload
def foo1(a: i32) -> i32:
    return a**4

@overload
def foo1(a: str) -> str:
    return "lpython-is-fun-" + a


@overload
def test1(a: i32) -> i32:
    return a + 20

@overload
def test1(a: bool) -> i32:
    if a:
        return 20
    return -20


@overload
def add(a: i32):
    print(a)

@overload
def add(a: i32, b: i32):
    print(a + b)

@overload
def add(a: i32, b: i64):
    print(i64(a) + b)


def check():
    assert foo(2) == 4
    assert foo1(2) == 16
    assert overload_testing2.foo2(2) == 8
    assert foo(2, 10) == 20
    assert foo1(2, 10) == -8
    assert overload_testing2.foo2(2, 10) == 12
    assert foo("hello") == "lpython-hello"
    assert foo1("hello") == "lpython-is-fun-hello"
    assert overload_testing2.foo2("hello") == "lpython-super-fun-hello"
    assert test(10) == 20
    assert test1(10) == 30
    assert overload_testing2.test2(10) == 40
    assert test(False) == -test(True) and test(True) == 10
    assert test1(False) == -test1(True) and test1(True) == 20
    assert overload_testing2.test2(True) == 30
    # We are testing the subroutine using print to test it actually works
    add(1, 2)
    add(3)
    i: i64
    i = i64(10)
    add(2, i)

check()
