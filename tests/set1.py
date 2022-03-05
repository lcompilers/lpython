def test_Set():
    a: set[i32]
    a = {1, 2, 3}
    a = {2, 3, 4, 5, 5}

    b: set[str]
    b = {"a", "b", "c"}
    # a = {-1, -2, "c"} -> semantic error for now
