from ltypes import i32

def test_bitnot():
    x: i32 = 5
    y: i32 = 0
    z: i32 = 2147483647
    w: i32 = -2147483648

    p: i32 = ~x
    q: i32 = ~y
    r: i32 = ~z
    s: i32 = ~w

    print(p)
    print(q)
    print(r)
    print(s)

    assert(p == -6)
    assert(q == -1)
    assert(r == -2147483648)
    assert(s == 2147483647)

def test_bitand():
    x: i32 = 5
    y: i32 = 3
    z: i32 = 2147483647
    w: i32 = -2147483648


    p: i32 = x & y
    q: i32 = y & z
    r: i32 = z & w
    s: i32 = x & w

    print(p)
    print(q)
    print(r)
    print(s)

    assert(p == 1)
    assert(q == 3)
    assert(r == 0)
    assert(s == 0)

def test_bitor():
    x: i32 = 5
    y: i32 = 3
    z: i32 = 2147483647
    w: i32 = -2147483648


    p: i32 = x | y
    q: i32 = y | z
    r: i32 = z | w
    s: i32 = x | w

    print(p)
    print(q)
    print(r)
    print(s)

    assert(p == 7)
    assert(q == 2147483647)
    assert(r == -1)
    assert(s == -2147483643)

def test_bitxor():
    x: i32 = 5
    y: i32 = 3
    z: i32 = 2147483647
    w: i32 = -2147483648


    p: i32 = x ^ y
    q: i32 = y ^ z
    r: i32 = z ^ w
    s: i32 = x ^ w

    print(p)
    print(q)
    print(r)
    print(s)

    assert(p == 6)
    assert(q == 2147483644)
    assert(r == -1)
    assert(s == -2147483643)

def test_left_shift():
    a: i32 = 4
    shift_amount: i32 = 2
    b: i32 = a << shift_amount
    print(b)
    assert b == 16

    a = -16
    shift_amount = 2
    b = a << shift_amount
    print(b)
    assert b == -64

def test_right_shift():
    a: i32 = 16
    shift_amount: i32 = 2
    b: i32 = a >> shift_amount
    print(b)
    assert b == 4

    a = -16
    shift_amount = 2
    b = a >> shift_amount
    print(b)
    assert b == -4

def main0():
    test_bitnot()
    test_bitand()
    test_bitor()
    test_bitxor()
    test_left_shift()
    test_right_shift()

main0()
