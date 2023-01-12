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


def g():
    a11: list[i32]
    b11: list[i32]
    a11 = [1, 2, 3, 4]
    b11 = [1, 2, 3, 4]
    assert a11 == b11
    a11.append(5)
    assert a11 != b11
    c11: list[i32] = []
    assert a11 != c11

    d11: list[str] = ['a', 'b', '^', '*']
    e11: list[str] = ['a', 'b', '^']
    assert d11 != e11
    e11.append('*')
    assert d11 == e11

    f11: list[tuple[i32, i32]] = []
    x: tuple[i32, i32]
    i: i32
    for i in range(10):
        x = (i, i+2)
        f11.append(x)

    g11: list[tuple[i32, i32]] = []
    for i in range(9):
        x = (i, i+2)
        g11.append(x)
    assert g11 != f11
    x = (9, 11)
    g11.append(x)
    assert g11 == f11


def check():
    f()
    g()


check()
