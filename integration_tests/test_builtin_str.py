from ltypes import f32, f64, i32

def test_str_int_float():
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
    assert str(12.1234)[:7] == "12.1234"

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
    xx = f32(12.322234)
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


def test_str_slice_step():
    s: str
    start: i32
    end: i32
    step: i32
    s = "abcdefghijk"
    start = 1
    end = 4
    step = 1

    # Check all possible combinations
    assert s[::] == "abcdefghijk"
    assert s[1:4:] == "bcd"
    assert s[:4:5] == "a"
    assert s[::-1] == "kjihgfedcba"
    assert s[3:12:3] == "dgj"
    assert s[1::3] == "behk"
    assert s[4::] == "efghijk"
    assert s[:5:] == "abcde"

    assert s[3:9:3] == "dg"
    assert s[10:3:-2] == "kige"
    assert s[-2:-10] == ""
    assert s[-3:-9:-3] == "if"
    assert s[-3:-10:-3] == "ifc"
    assert s[start:end:step] == "bcd"
    assert s[start:2*end-3:step] == "bcde"
    assert s[start:2*end-3:-step] == ""



def test_issue_883():
    s: str
    s = "abcde"
    c: str
    d: str
    d = "edcba"
    i: i32 = 0
    for c in s[::-1]:
        print(c)
        assert c == d[i]
        i += 1


test_str_int_float()
str_conv_for_variables()
test_str_slice_step()
test_issue_883()
