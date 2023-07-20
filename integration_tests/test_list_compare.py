from lpython import i32, f64

def test_list_compare():
    l1: list[i32] = [1, 2, 3]
    l2: list[i32] = [1, 2, 3, 4]
    l3: list[tuple[i32, f64, str]] = [(1, 2.0, 'a'), (3, 4.0, 'b')]
    l4: list[tuple[i32, f64, str]] = [(1, 3.0, 'a')]
    l5: list[list[str]] = [[''], ['']]
    l6: list[str] = []
    l7: list[str] = []
    t1: tuple[i32, i32]
    t2: tuple[i32, i32]
    i: i32

    assert l1 < l2 and l1 <= l2
    assert not l1 > l2 and not l1 >= l2
    i = l2.pop()
    i = l2.pop()
    assert l2 < l1 and l1 > l2 and l1 >= l2
    assert not (l1 < l2)

    l1 = [3, 4, 5]
    l2 = [1, 6, 7]
    assert l1 > l2 and l1 >= l2
    assert not l1 < l2 and not l1 <= l2

    l1 = l2
    assert l1 == l2 and l1 <= l2 and l1 >= l2
    assert not l1 < l2 and not l1 > l2

    assert l4 > l3 and l4 >= l3
    l4[0] = l3[0]
    assert l4 < l3

    for i in range(0, 10):
        if i % 2 == 0:
            l6.append('a')
        else:
            l7.append('a')
        l5[0] = l6
        l5[1] = l7
        if i % 2 == 0:
            assert l5[1 - i % 2] < l5[i % 2]
            assert l5[1 - i % 2] <= l5[i % 2]
            assert not l5[1 - i % 2] > l5[i % 2]
            assert not l5[1 - i % 2] >= l5[i % 2]

    t1 = (1, 2)
    t2 = (2, 3)
    assert t1 < t2 and t1 <= t2
    assert not t1 > t2 and not t1 >= t2

test_list_compare()