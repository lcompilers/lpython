def test_List():
    a: i32
    b: i32

    a = [1, 2, 3]
    a = [-3, -2, -1]
    a = ["a", "b", "c"]
    a = [[1, 2, 3], [4, 5, 6]]
    # a = [-2, -1, 0.45] -> semantic error

    b = a[2]
