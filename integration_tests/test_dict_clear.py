def test_clear():
    a: dict[i32, i32] = {1:1, 2:2}

    a.clear()
    a[3] = 3

    assert len(a) == 1
    assert 3 in a

    b: dict[str, str] = {'a':'a', 'b':'b'}

    b.clear()
    b['c'] = 'c'

    assert len(b) == 1
    assert 'c' in b

test_clear()
