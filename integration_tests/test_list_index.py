from lpython import i32, f64

def test_list_index():
    i: i32
    x: list[i32] = []
    y: list[str] = []
    z: list[tuple[i32, str, f64]] = []
    
    for i in range(-5, 0):
        x.append(i)
        assert x.index(i) == len(x)-1
        x.append(i)
        assert x.index(i) == len(x)-2
        x.remove(i)
        assert x.index(i) == len(x)-1

    assert x == [-5, -4, -3, -2, -1]

    for i in range(-5, 0):
        x.append(i)
        assert x.index(i) == 0
        x.remove(i)
        assert x.index(i) == len(x)-1

    # str
    y = ['a', 'abc', 'a', 'b', 'abc']
    assert y.index('a') == 0
    assert y.index('abc') == 1

    # tuple, float
    z = [(i32(1), 'a', f64(2.01)), (i32(-1), 'b', f64(2)), (i32(1), 'a', f64(2.02))]
    assert z.index((i32(1), 'a', f64(2.01))) == 0
    z.insert(0, (i32(1), 'a', f64(2)))
    assert z.index((i32(1), 'a', f64(2.00))) == 0
    z.append((i32(1), 'a', f64(2.00)))
    assert z.index((i32(1), 'a', f64(2))) == 0
    z.remove((i32(1), 'a', f64(2)))
    assert z.index((i32(1), 'a', f64(2.00))) == 3

test_list_index()