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
    assert str("just a str") == "just a str"
    assert str(12.1234) == "12.1234"

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
    xx: f32
    xx = 12.322234
    assert str(xx) == "12.322234"
    yy : f64
    yy = 12.322234
    assert str(yy) == "12.322234"
    bool_t :bool
    bool_t = True 
    assert str(bool_t) == "True"
    bool_t = False
    assert str(bool_t) == "False"
    str_t :str 
    str_t = "just a str"
    assert str(str_t) == str_t


test_str_int()
str_conv_for_variables()
