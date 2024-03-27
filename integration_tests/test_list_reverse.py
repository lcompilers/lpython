from lpython import i32, f64

def test_list_reverse():
    l1: list[i32] = []
    l2: list[i32] = []
    l3: list[f64] = []
    l4: list[f64] = []
    l5: list[str] = []
    l6: list[str] = []
    l7: list[str] = []
    l8: list[list[i32]] = []
    l9: list[tuple[i32, f64, str]] = []
    i: i32
    j: f64
    s: str

    l1 = [1, 2, 3]
    l1.reverse()
    assert l1 == [3, 2, 1]

    l1 = []
    for i in range(10):
        l1.reverse()
        l1.append(i)
        l2.insert(0, i)
        l1.reverse()
        assert l1 == l2

    j = 0.0
    while j < 2.1:
        l3.reverse()
        l3.append(j)
        l4.insert(0, j)
        l3.insert(0, j + 1.0)
        l4.append(j + 1.0)
        l3.reverse()
        assert l3 == l4
        j += 0.1

    l5 = ["abcd", "efgh", "ijkl"]
    for s in l5:
        l6.reverse()
        l6.insert(0, s)
        l7.append(s)
        l6.reverse()
        assert l6 == l7

    l8 = [[1, 2], [3, 4, 5], [6, 7, 8, 9], [10]]
    l8.reverse()
    assert l8 == [[10], [6, 7, 8, 9], [3, 4, 5], [1, 2]]

    l9 = [(1, 2.0, "abc"), (3, 4.0, "def"), (5, 6.0, "ghi")]
    l9.reverse()
    assert l9 == [(5, 6.0, "ghi"), (3, 4.0, "def"), (1, 2.0, "abc")]


test_list_reverse()
