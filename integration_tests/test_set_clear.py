def test_clear():
    a: set[i32] = {1, 2}

    a.clear()
    # a.add(3)

    assert len(a) == 0
    # a.remove(3)
    # assert len(a) == 0

    b: set[str] = {'a', 'b'}

    b.clear()
    # b.add('c')

    assert len(b) == 0
    # b.remove('c')
    # assert len(b) == 0


test_clear()
