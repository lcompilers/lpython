def test_pow():
    a: i32
    a = pow(2, 2)


def test_pow_1(a: i32, b: i32) -> i32:
    res: i32
    res = pow(a, b)
    return res

def main0():
    test_pow()
    c: i32
    c = test_pow_1(1, 2)

main0()
