from lpython import i32, f64

def test_builtin_type():
    i: i32 = 42
    f: f64 = 64.0
    s: str = "Hello, LPython!"
    l: list[i32] = [1, 2, 3, 4, 5]
    d: dict[str, i32] = {"a": 1, "b": 2, "c": 3}
    t: tuple[str, i32] = ("a", 1)
    st: set[i32] = {1, 2, 3, 4}
    res: str = ""

    res = str(type(i))
    print(res)
    assert res == "<class 'int'>"
    res = str(type(f))
    print(res)
    assert res == "<class 'float'>"
    res = str(type(s))
    print(res)
    assert res == "<class 'str'>"
    res = str(type(l))
    print(res)
    assert res == "<class 'list'>"
    res = str(type(d))
    print(res)
    assert res == "<class 'dict'>"
    res = str(type(t))
    print(res)
    assert res == "<class 'tuple'>"
    res = str(type(st))
    print(res)
    assert res == "<class 'set'>"
    

test_builtin_type()
