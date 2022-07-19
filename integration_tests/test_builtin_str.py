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

def str_conv_for_variables():
    x: i32
    x = 123
    assert "123" == str(x)
    x = 12345
    assert "12345" == str(x)
    x = -12
    assert "-12" == str(x)
    x = -121212
    assert "-121212" == str(x)

test_str_int()
str_conv_for_variables()
