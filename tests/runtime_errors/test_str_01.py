from ltypes import i32


def test_issue_1536():
    x: str
    x = "ABC"
    l: i32
    l = -4
    print(x[l])

test_issue_1536()
