from lpython import i32, f64, Const

def test_builtin_type():
    i: i32 = 42
    f: f64 = 64
    s: str = "Hello, LPython!"
    l: list[i32] = [1, 2, 3, 4, 5]
    d: dict[str, i32] = {"a": 1, "b": 2, "c": 3}
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

test_builtin_type()
