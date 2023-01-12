from ltypes import i32, i64, i16

def main0():
    s: str = 'aabbcc'; c: str
    for c in s:
        print(c)

def test_issue_770():
    i: i32; res: i64 = int(0)
    for i in range(10):
        res += int(i)
    assert res == int(45)

def test_increment():
    i: i16
    sumi: i16 = i16(0)
    for i in range(i16(15), i16(-1), i16(-1)):
        sumi += i
    print(sumi)
    assert sumi == i16(120)

main0()
test_issue_770()
test_increment()
