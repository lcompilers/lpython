from lpython import i32

def test_set():
    s: set[i32]
    s = {1, 2, 22, 2, -1, 1}
    s2: set[str]
    s2 = {'a', 'b', 'cd', 'b', 'abc', 'a'}
    assert len(s2) == 4

test_set()
