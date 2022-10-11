from ltypes import i32

def test_list_section():
    x: list[i32] = []
    y: list[i32]
    y = x[:]
    assert len(y) == 0
    y = x[0:0]
    assert len(y) == 0

    x = [1, 2, 3]
    y = x[0:10]
    assert len(x) == len(y)
    x.clear()

    i: i32
    for i in range(50):
        x.append(i)

    y = x[15:35]
    j: i32 = 0
    for i in range(15, 35):
        assert x[i] == y[j]
        j += 1

    y = x[:50]
    for i in range(50):
        assert i == y[i]

    y = x[10:]
    j = 0
    for i in range(10, 50):
        assert x[i] == y[j]
        j += 1

    y = x[::2]
    j = 0
    for i in range(0, len(x), 2):
        assert x[i] == y[j]
        j += 1

test_list_section()
