from lpython import i32, f64

def test_list_index2():
    # test optional start and end parameters
    i: i32
    x: list[i32] = []
    y: list[str] = []
    z: list[tuple[i32, str, f64]] = []

    x = [1, 2, 3, 2]
    assert x.index(2, 0) == 1
    assert x.index(2, 1) == 1
    assert x.index(2, 2) == 3
    assert x.index(2, 1, 4) == 1
    assert x.index(2, 2, 4) == 3

    x = []
    for i in range(-5, 0):
        x.append(i)
        assert x.index(i, 0) == len(x) - 1
        assert x.index(i, 0, len(x)) == len(x) - 1
        x.append(i)
        assert x.index(i, 0) == len(x) - 2
        assert x.index(i, 0, len(x)) == len(x) - 2
        assert x.index(i, len(x) - 1) == len(x) - 1
        assert x.index(i, len(x) - 1, len(x)) == len(x) - 1
        x.remove(i)
        assert x.index(i, 0) == len(x) - 1
        assert x.index(i, 0, len(x)) == len(x) - 1

    assert x == [-5, -4, -3, -2, -1]

    # str
    y = ['a', 'abc', 'a', 'b', 'abc']
    assert y.index('a', 0) == 0
    assert y.index('a', 0, 1) == 0
    assert y.index('a', 1) == 2
    assert y.index('a', 1, 3) == 2
    assert y.index('abc', 0) == 1
    assert y.index('abc', 1) == 1
    assert y.index('abc', 2) == 4

    # tuple, float
    z = [(1, 'a', 2.01), (-1, 'b', 2.0), (1, 'a', 2.02)]
    assert z.index((1, 'a', 2.01), 0) == 0
    assert z.index((1, 'a', 2.01), 0, 1) == 0
    z.insert(0, (1, 'a', 2.0))
    assert z.index((1, 'a', 2.0), 0) == 0
    assert z.index((-1, 'b', 2.0), 1) == 2
    z.insert(0, (1, 'a', 2.0))
    assert z.index((-1, 'b', 2.0), 1) == 3
    assert z.index((-1, 'b', 2.0), 1, 4) == 3
    assert z.index((-1, 'b', 2.0), 1, 5) == 3

test_list_index2()