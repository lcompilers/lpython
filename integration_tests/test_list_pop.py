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
    assert x == (1, 2.0)

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

    total = 10
    l2 = []
    for i in range(total):
        l2.append((i, f64(i * i)))
    j = 0
    for i in range(total):
        assert l2.pop(j - i) == (i, f64(i * i))
        j += 1
    assert len(l2) == 0

    # list.pop on list constant
    assert [1, 2, 3, 4, 5].pop() == 5
    print([1, 2, 3, 4, 5].pop())

    assert [1, 2, 3, 4, 5].pop(3) == 4
    print([1, 2, 3, 4, 5].pop(3))

    index: i32 = 1
    assert [1, 2, 3, 4, 5].pop(index) == 2
    print([1, 2, 3, 4, 5].pop(index))

    element_1: i32 = [1, 2, 3, 4, 5].pop()
    assert element_1 == 5
    print(element_1)

    element_2: i32 = [1, 2, 3, 4, 5].pop(2)
    assert element_2 == 3
    print(element_2)

    a: i32 = 5
    b: i32 = 3

    assert [(1, 2), (3, 4), (5, 6)].pop(a//b) == (3, 4)
    print([(1, 2), (3, 4), (5, 6)].pop(a//b)) 

    assert [["a", "b"], ["c", "d"], ["e", "f"]].pop() == ["e", "f"]
    print([["a", "b"], ["c", "d"], ["e", "f"]].pop())

test_list_pop()