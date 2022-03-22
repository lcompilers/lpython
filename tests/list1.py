def test_List():
    a: list[i32]
    a = [1, 2, 3]
    a = [-3, -2, -1]
    # a = [-2, -1, 0.45] -> semantic error

    b: list[str]
    b = ["a", "b", "c"]

    c: list[list[i32], list[i32]]
    c = [[1, 2, 3], [4, 5, 6]]

    d: i32
    d = a[2]

    # ragged list
    e: list[list[str], list[str]]
    e = [['a', 'b', 'c'], ['d', 'e']]
