def test_Set():
    a: set[i32]
    a = {1, 2, 3}
    a = {2, 3, 4, 5, 5}

    a.add(9)
    a.remove(4)

    b: set[str]
    b = {"a", "b", "c"}
    # a = {-1, -2, "c"} -> semantic error for now

    s: str
    s = b.pop()

s1: set[str] = {"a", "b", "c", "a"}
s2: set[i32] = {1, 2, 3, 1}
s3: set[tuple[i32, i32]] = {(1, 2), (2, 3), (4, 5)}
