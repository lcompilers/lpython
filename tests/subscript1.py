def test_subscript():
    s: str
    s = 'abc'
    s = s[0]
    s = s[1:2]

    A: i32[5]
    B: i32[2]
    i: i32
    i = A[0]
    B = A[1:3]
