from lpython import i32

def test_list_count():
    i: i32
    x: list[i32] = []
    
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
    
    while len(x)>0:
        i = x[-1]
        x.remove(i)
        assert x.count(i) == 0

    assert len(x) == 0
    assert x.count(0) == 0

test_list_count()
