def test_ord():
    i: i32
    s: str
    s = "1"
    i = ord(s)
    assert i == 49
    assert ord("1") == 49


def test_chr():
   i: i32
   i = 48
   s: str
   s = chr(i)
   assert s == "0"
   assert chr(48) == "0"


test_ord()
test_chr()
