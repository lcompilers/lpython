def test_str_int():
    s: str
    s = str(356)
    assert s == "356"
    s = str(-567)
    assert s == "-567"
    assert str(4) == "4"
    assert str(-5) == "-5"
