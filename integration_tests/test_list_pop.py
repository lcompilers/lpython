from lpython import i32, f64

def test_list_pop():
    l1: list[i32]
    l2: list[tuple[i32, f64]]
    l3: list[list[str]]
    i: i32
    j: i32
    total: i32
    x: tuple[i32, f64]

    l1 = [1, 2, 3]
    assert l1.pop() == 3
    assert l1 == [1, 2]

    l1 = []
    total = 10
    for i in range(total):
        l1.append(i)
        if i % 2 == 1:
            assert l1.pop() == i
    for i in range(total // 2):
        assert l1[i] == 2 * i

    l2 = [(1, 2.0)]
    x = l2.pop()
    assert x == (1, 2.0)
    assert len(l2) == 0
    l2.append((2, 3.0))

    l3 = []
    for i in range(total):
        l3.insert(0, ["a"])
        for j in range(len(l3)):
            l3[j] += ["a"]
    while len(l3) > 0:
        total = len(l3)
        assert len(l3.pop()) == total + 1
    assert len(l3) == 0

    l1 = [0, 1, 2, 3, 4]
    assert l1.pop(3) == 3
    assert l1.pop(0) == 0
    assert l1.pop(len(l1) - 1) == 4
    assert l1 == [1, 2]

    total = 10
    l1 = []
    for i in range(total):
        l1.append(i)
    j = 0
    for i in range(total):
        assert l1.pop(j - i) == i
        j += 1
    assert len(l1) == 0

    l2 = [(1, 1.0), (2, 4.0)]
    x = l2.pop(0)
    # print(x)        # incorrect
    # print(l2)       # correct

test_list_pop()