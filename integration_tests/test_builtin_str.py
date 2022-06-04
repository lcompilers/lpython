def test_str_int():
    s: str
    s = str(356)
    assert s == "356"
    s = str(-567)
    assert s == "-567"
    assert str(4) == "4"
    assert str(-5) == "-5"
    assert str() == ""
    assert str("1234") == "1234"
    assert str(False) == "False"
    assert str(True) == "True"

test_str_int()
