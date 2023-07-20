from lpython import i32, f64

def main0():
    a: i32 = 4
    b: i32 = 3
    c: i32 = 12
    assert a * b == c, a * b

    d: f64 = 0.4
    e: f64 = 2.5
    f: f64 = 1.0
    assert abs((d * e) - f) <= 1e-6, abs((d * e) - f)

    assert a == b + 1, "Failed: a == b + 1"

main0()
