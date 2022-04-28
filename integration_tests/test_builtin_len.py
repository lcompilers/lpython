def test_len():
    s: str
    s = "abcd"
    assert len(s) == 4
    s = ''
    assert len(s) == 0
    assert len("abcd") == 4
    assert len("") == 0
    t: str
    t = "efg"
    s = "abcd" + t
    i: i32
    i = len(s)
    assert i == 7
    i = len("abc" + "def")
    assert i == 6

test_len()
