def main0():
    s: str = 'aabbcc'; c: str
    for c in s:
        print(c)

def test_issue_770():
    i: i32; res: i64 = int(0)
    for i in range(10):
        res += int(i)
    assert res == int(45)

main0()
test_issue_770()
