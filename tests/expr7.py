def test_pow():
    a: f64
    a = pow(2, 2)


def test_pow_1(a: i32, b: i32) -> f64:
    res: f64
    res = pow(a, b)
    return res

def main0():
    test_pow()
    c: f64
    c = test_pow_1(1, 2)

main0()
