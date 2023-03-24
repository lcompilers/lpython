from lpython import c32

def test_issue_1524():
    y: c32
    y = complex(5)+100
    print(y)

test_issue_1524()
