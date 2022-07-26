def main0():
    s: str
    s = 'aabbcc'
    c: str
    for c in s:
        print(c)

def test_issue_770():
    i: i64
    res: i64
    res = 0
    for i in range(10):
        res += i
    assert res == 45

main0()
test_issue_770()
