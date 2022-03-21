from overload_testing import foo, test
from ltypes import overload, i32
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

check()
