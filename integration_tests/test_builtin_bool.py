def test_bool():
    a: i32
    a = 34
    assert bool(a)
    a = 0
    assert bool(a) == False
    # assert bool(-1)
    # assert bool(0) == False
