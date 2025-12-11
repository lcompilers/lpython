from lpython import i32


def test_list_concat():
    x: list[i32] = []
    y: list[i32] = []
    z: list[i32] = x + y
    i: i32
    assert len(z) == 0

    x = [1, 2, 3]
    z = x + y
    for i in range(1, 4):
        assert z[i - 1] == i

    x.clear()
    y = [6, 7, 8]
    z = x + y
    for i in range(1, 4):
        assert z[i - 1] == i + 5

    x = [1, 2, 3, 4, 5]
    z = x + y
    for i in range(1, 9):
        assert z[i - 1] == i

    x.clear()
    y.clear()
    for i in range(9, 51):
        x.append(i)
    for i in range(51, 101):
        y.append(i)

    z = z + x + y
    x[0] = 0
    x[1] = 0
    y.clear()
    for i in range(1, 100):
        assert z[i - 1] == i

    c: list[str]
    d: list[str]
    c = ["a", "b"]
    d = ["c", "d", "e"]
    c += d
    assert len(c) == 5
    for i in range(5):
        assert ord(c[i]) - ord("a") == i


test_list_concat()
