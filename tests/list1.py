def test_List():
    a: list[i32]
    a = [1, 2, 3]
    a = [-3, -2, -1]

    b: list[str]
    b = ["a", "b", "c"]

    c: i32
    c = [[1, 2, 3], [4, 5, 6]]
    # a = [-2, -1, 0.45] -> semantic error
    c = a[2]
