from ltypes import i64

def test_bitnot():
    x: i64 = i64(123)
    y: i64 = i64(456)
    z: i64 = i64(115292150460684)
    w: i64 = i64(-115292150460685)

    p: i64 = ~x
    q: i64 = ~y
    r: i64 = ~z
    s: i64 = ~w

    print(p)
    print(q)
    print(r)
    print(s)

    assert(p == i64(-124))
    assert(q == i64(-457))
    assert(r == i64(-115292150460685))
    assert(s == i64(115292150460684))

def test_bitand():
    x: i64 = i64(741)
    y: i64 = i64(982)
    z: i64 = i64(115292150460684)
    w: i64 = i64(-115292150460685)

    p: i64 = x & y
    q: i64 = y & z
    r: i64 = z & w
    s: i64 = x & w

    print(p)
    print(q)
    print(r)
    print(s)

    assert(p == i64(708))
    assert(q == i64(260))
    assert(r == i64(0))
    assert(s == i64(737))

def test_bitor():
    x: i64 = i64(741)
    y: i64 = i64(982)
    z: i64 = i64(115292150460684)
    w: i64 = i64(-115292150460685)

    p: i64 = x | y
    q: i64 = y | z
    r: i64 = z | w
    s: i64 = x | w

    print(p)
    print(q)
    print(r)
    print(s)

    assert(p == i64(1015))
    assert(q == i64(115292150461406))
    assert(r == i64(-1))
    assert(s == i64(-115292150460681))

def test_bitxor():
    x: i64 = i64(741)
    y: i64 = i64(982)
    z: i64 = i64(115292150460684)
    w: i64 = i64(-115292150460685)

    p: i64 = x ^ y
    q: i64 = y ^ z
    r: i64 = z ^ w
    s: i64 = x ^ w

    print(p)
    print(q)
    print(r)
    print(s)

    assert(p == i64(307))
    assert(q == i64(115292150461146))
    assert(r == i64(-1))
    assert(s == i64(-115292150461418))

def test_left_shift():
    a: i64 = i64(4294967296)
    shift_amount: i64 = i64(2)
    b: i64 = a << shift_amount
    print(b)
    assert b == i64(17179869184)

    a = i64(-4294967296)
    shift_amount = i64(2)
    b = a << shift_amount
    print(b)
    assert b == i64(-17179869184)

def test_right_shift():
    a: i64 = i64(4294967296)
    shift_amount: i64 = i64(2)
    b: i64 = a >> shift_amount
    print(b)
    assert b == i64(1073741824)

    a = i64(-4294967296)
    shift_amount = i64(2)
    b = a >> shift_amount
    print(b)
    assert b == i64(-1073741824)

def main0():
    test_bitnot()
    test_bitand()
    test_bitor()
    test_bitxor()
    test_left_shift()
    test_right_shift()

main0()
