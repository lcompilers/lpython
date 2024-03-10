from lpython import i32, f64, list, str, dict, Const

def test_builtin_type():
    i: i32 = 42
    s: str = "Hello, LPython!"
    l: list[i32] = [1, 2, 3, 4, 5]
    d: dict[str, i32] = {"a": 1, "b": 2, "c": 3}
    CONST_LIST: Const[list[f64]] = [12.22, 14.63, 33.82, 19.18]

    assert type(i) == "<type 'i32'>"
    assert type(s) == "<type 'str'>"
    assert type(l) == "<type 'list[i32]'>"
    assert type(d) == "<type 'dict[str, i32]'>"
    assert type(CONST_LIST) == "<type 'Const[list[f64]]'>"

    assert type(type(i)) == "<type 'type'>"
    assert type(type(CONST_LIST)) == "<type 'type'>"

test_builtin_type()
