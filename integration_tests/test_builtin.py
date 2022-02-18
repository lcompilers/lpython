from ltypes import i32

def test_ord():
    i: i32
    s: str
    s = "1"
    i = ord(s)
    print(i)


def test_chr():
    i: i32
    i = 48
    s: str
    s = chr(i)
    print(s)


test_ord()
test_chr()
