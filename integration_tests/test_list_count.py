from lpython import i32, f64

def test_list_count():
    i: i32
    x: list[i32] = []
    y: list[str] = []
    z: list[tuple[i32, str, f64]] = []
    
    for i in range(-5, 0):
        assert x.count(i) == 0
        x.append(i)
        assert x.count(i) == 1
        x.append(i)
        assert x.count(i) == 2
        x.remove(i)
        assert x.count(i) == 1

    assert x == [-5, -4, -3, -2, -1]

    for i in range(0, 5):
        assert x.count(i) == 0
        x.append(i)
        assert x.count(i) == 1

    assert x == [-5, -4, -3, -2, -1, 0, 1, 2, 3, 4]
    
    while len(x) > 0:
        i = x[-1]
        x.remove(i)
        assert x.count(i) == 0

    assert len(x) == 0
    assert x.count(0) == 0

    # str
    assert y.count('a') == 0
    y = ['a', 'abc', 'a', 'b']
    assert y.count('a') == 2
    y.append('a')
    assert y.count('a') == 3
    y.remove('a')
    assert y.count('a') == 2

    # tuple, float
    assert z.count((i32(-1), 'b', f64(2))) == 0
    z = [(i32(1), 'a', f64(2.01)), (i32(-1), 'b', f64(2)), (i32(1), 'a', f64(2.02))]
    assert z.count((i32(1), 'a', f64(2.00))) == 0
    assert z.count((i32(1), 'a', f64(2.01))) == 1
    z.append((i32(1), 'a', f64(2)))
    z.append((i32(1), 'a', f64(2.00)))
    assert z.count((i32(1), 'a', f64(2))) == 2
    z.remove((i32(1), 'a', f64(2)))
    assert z.count((i32(1), 'a', f64(2.00))) == 1

test_list_count()
