from lpython import f64, i32

def fill_list_i32(size: i32) -> list[i32]:
    aarg: list[i32] = [0, 1, 2, 3, 4]
    i: i32
    for i in range(10):
        aarg.append(i + 5)
    return aarg


def test_list_01():
    a: list[i32] = []
    f: list[f64] = [1.0, 2.0, 3.0, 4.0, 5.0]
    i: i32

    a = fill_list_i32(10)

    for i in range(10):
        f.append(float(i + 6))

    for i in range(15):
        assert (f[i] - f64(a[i])) == 1.0

    for i in range(15):
        f[i] = f[i] + f64(i)

    for i in range(15):
        assert (f[i] - f64(a[i])) == (f64(i) + 1.0)

def test_list_02():
    x: list[i32] = [1, 2]
    y: list[i32] = []

    i: i32
    for i in range(3, 50):
        x.append(i + 29)

    for i in x:
        y.append(i)

    j: i32
    for j in range(len(y)):
        assert x[j] == y[j]

# Negative Indexing
def test_list_03():
    x: list[f64] = []

    i: i32
    for i in range(2):
        x.append(float(i))

    assert x[1] == x[-1]
    assert x[0] == x[-2]
    assert x[-1] == 1.0
    assert x[-2] == 0.0

    size: i32 = 2
    for i in range(100):
        x.append((i * size)/2)
        size = len(x)

    for i in range(size):
        assert x[i] == x[((i-len(x)) + size) % size]


def test_issue_1681():
    a: list[i32] = [2, 3, 4]
    a = [1, 2, 3]
    assert len(a) == 3 and a[0] == 1 and a[1] == 2 and a[2] == 3
    a = []
    assert len(a) == 0
    a = [1]
    assert len(a) == 1 and a[0] == 1


def tests():
    test_list_01()
    test_list_02()
    test_list_03()
    test_issue_1681()

tests()
