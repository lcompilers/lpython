from ltypes import i32, f64

def test_pow():
    # TODO: the commented tests should also work
    a: i32
    b: i32
    eps: f64
    eps = 1e-12
    a = 2
    b = 5
    assert pow(a, b) == 32
    a = 6
    b = 3
    assert pow(a, b) == 216
    a = 2
    b = 0
    assert pow(a, b) == 1
    a = 2
    b = -1
    print(pow(a, b))
    a = 6
    b = -4
    print(pow(a, b))

    a1: f64
    a2: f64
    a1 = 4.5
    a2 = 2.3
    assert abs(pow(a1, a2) - 31.7971929089206) < eps
    assert abs(pow(a2, a1) - 42.43998894277659) < eps

    x: i32
    x = 3
    y: f64
    y = 2.3
    assert abs(pow(x, y) - 12.513502532843182) < eps
    assert abs(pow(y, x) - 12.166999999999998) < eps
    assert abs(pow(x, 5.5) - 420.8883462392372) < eps

    assert abs(pow(2, -1) - 0.5) < eps
    assert abs(pow(6, -4) - 0.0007716049382716049) < eps
    assert abs(pow(-3, -5) + 0.00411522633744856) < eps
    assert abs(pow(6, -4) - 0.0007716049382716049) < eps
    assert abs(pow(4.5, 2.3) - 31.7971929089206) < eps
    assert abs(pow(2.3, 0.0) - 1.0) < eps
    assert abs(pow(2.3, -1.5) - 0.2866871623459944) < eps
    assert abs(pow(2, 3.4) - 10.556063286183154) < eps
    assert abs(pow(2, -3.4) - 0.09473228540689989) < eps
    assert abs(pow(3.4, 9) - 60716.99276646398) < eps
    assert abs(pow(0.0, 53) - 0.0) < eps
    assert pow(4, 2) == 16
    # assert abs(pow(-4235.0, 52) - 3.948003805985264e+188) < eps


test_pow()
