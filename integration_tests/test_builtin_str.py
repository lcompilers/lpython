from lpython_builtin import str_int

def test_str_int():
    s: str
    s = str_int(356)
    assert s == "356"
    s = str_int(-567)
    assert s == "-567"
