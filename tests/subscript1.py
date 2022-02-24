def test_subscript():
    s: str
    s = 'abc'
    s = s[0]
    s = s[1:2]
    s = s[::]
    s = s[::-1]
    s = s[::2]
    s = s[1:88:1]
    s = s[:1:-4]
    s = s[-89::4]
    s = s[-3:-3:-3]
    s = s[2:3:]


    A: i32[5]
    B: i32[2]
    i: i32
    i = A[0]
    B = A[1:3]
    B = A[1:2:3]
