from lpython import i32, i64, f32, f64, c32

def test_pow():
    # TODO: the commented tests should also work
    a: i32
    b: i32
    eps: f64
    eps = 1e-12
    a = 2
    b = 5
    assert i32(pow(a, b)) == 32 # noqa
    a = 6
    b = 3
    assert i32(pow(a, b)) == 216
    a = 2
    b = 0
    assert i32(pow(a, b)) == 1
    a = 2
    b = -1
    # assert abs(pow(a, b) - 0.5) < eps
    a = 6
    b = -4
    # assert abs(pow(a, b) - 0.0007716049382716049) < eps

    i1: i64
    i2: i64
    i1 = i64(2)
    i2 = i64(5)
    assert i64(pow(i1, i2)) == i64(32)
    i1 = i64(6)
    i2 = -i64(3)
    # assert abs(pow(i1, i2) - 0.004629629629629629) < eps

    f1: f32
    f2: f32
    p: f32
    f1 = f32(525346/66456)
    f2 = f32(3.0)
    p = pow(f1, f2)

    f1 = pow(a, f2) # (i32, f32)
    f1 = pow(f2, a) # (f32, i32)

    b1: bool = True
    b2: bool = False
    assert pow(b1, b2) == 1
    assert pow(b2, b1) == 0
    assert pow(b1, b2) == 1
    assert pow(False, False) == 1

    a1: f64 = 4.5
    a2: f64 = 2.3
    assert abs(pow(a1, a2) - 31.7971929089206) < eps
    assert abs(pow(a2, a1) - 42.43998894277659) < eps

    x: i32 = 3
    y: f64 = 2.3
    assert abs(pow(x, y) - 12.513502532843182) < eps
    assert abs(pow(y, x) - 12.166999999999998) < eps
    assert abs(pow(x, 5.5) - 420.8883462392372) < eps

    assert abs(pow(i64(2), -i64(1)) - 0.5) < eps
    assert abs(pow(i64(6), -i64(4)) - 0.0007716049382716049) < eps
    assert abs(pow(-i64(3), -i64(5)) + 0.00411522633744856) < eps
    assert abs(pow(i64(6), -i64(4)) - 0.0007716049382716049) < eps
    assert abs(pow(4.5, 2.3) - 31.7971929089206) < eps
    assert abs(pow(2.3, 0.0) - 1.0) < eps
    assert abs(pow(2.3, -1.5) - 0.2866871623459944) < eps
    assert abs(pow(2, 3.4) - 10.556063286183154) < eps
    assert abs(pow(2, -3.4) - 0.09473228540689989) < eps
    assert abs(pow(3.4, 9) - 60716.99276646398) < eps
    assert abs(pow(0.0, 53) - 0.0) < eps
    assert i32(pow(4, 2)) == 16
    assert abs(pow(-4235.0, 52) - 3.948003805985264e+188) < eps

    i: i64
    i = i64(7)
    j: i64
    j = i64(2)
    k: i64
    k = i64(5)
    assert pow(i, j, k) == i64(4)
    assert pow(102, 3, 121) == 38

    c1: c32
    c1 = c32(complex(4, 5))
    c1 = pow(c1, 4)

test_pow()
