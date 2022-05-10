def test_List():
    a: list[i32]
    a = [1, 2, 3]
    a = [-3, -2, -1]
    # a = [-2, -1, 0.45] -> semantic error

    b: list[str]
    b = ["a", "b", "c"]

    c: list[list[i32]]
    c = [[1, 2, 3], [4, 5, 6]]

    d: i32
    d = a[2]

    # ragged list
    e: list[list[str]]
    e = [['a', 'b', 'c'], ['d', 'e']]

    a.append(10)
    a.remove(1)
    a.insert(2, 13)
    a = a[0:2]
    d = a.pop()
    d = a.pop(2)
    a += [4, 5]
    a = [6, 7] + a
