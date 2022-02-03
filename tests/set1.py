def test_Set():
    a: i32
    b: i32

    a = {1, 2, 3}
    a = {2, 3, 4, 5, 5}
    a = {"a", "b", "c"}
    # a = {-1, -2, "c"} -> semantic error for now
