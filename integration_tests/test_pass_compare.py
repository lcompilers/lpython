from ltypes import i32

def f():
    a11: tuple[i32, i32]
    b11: tuple[i32, i32]
    a11 = (1, 2)
    b11 = (1, 2)
    assert a11 == b11
    c11: tuple[i32, i32]
    c11 = (1, 3)
    assert a11 != c11 and c11 != b11
    a: tuple[tuple[i32, i32], str, bool]
    b: tuple[tuple[i32, i32], str, bool]
    a = (a11, 'ok', True)
    b = (b11, 'ok', True)
    assert a == b
    b = (b11, 'notok', True)
    assert a != b


f()
