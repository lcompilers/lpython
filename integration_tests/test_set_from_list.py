from lpython import i32


def test_set():
    s: set[i32]
    s = set([1, 2, 2, 2, -1, 1, 1, 3])
    assert len(s) == 4

    s2: set[str]
    s2 = set(["a", "b", "b", "abc", "a"])
    assert len(s2) == 3


test_set()
