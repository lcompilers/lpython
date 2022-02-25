def test_len():
    s: str
    s = "abcd"
    assert len(s) == 4
    s = ''
    assert len(s) == 0
    assert len("abcd") == 4
    assert len("") == 0
