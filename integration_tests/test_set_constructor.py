def test_empty_set():
    a: set[i32] = set()
    assert len(a) == 0
    a.add(2)
    a.remove(2)
    a.add(3)
    assert a.pop() == 3

    b: set[str] = set()

    assert len(b) == 0
    b.add('a')
    b.remove('a')
    b.add('b')
    assert b.pop() == 3

test_empty_set()
