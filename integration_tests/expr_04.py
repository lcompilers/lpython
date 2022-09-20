def main0():
    i: i32
    sum: i32
    sum = 0
    for i in range(0, 10):
        if i == 0:
            continue
        sum += i
        if i > 5:
            break
    assert sum == 21


def test_issue_255():
    i: f64
    i = 300.27
    eps: f64
    eps = 1e-12
    assert abs(i//1.0 - 300.0) < eps


def test_floor_div():
    a: i32
    b: i32
    a = 5
    b = 2
    assert a//b == 2
    a = -5
    assert a//b == -3
    x: f32
    eps: f64
    eps = 1e-12
    x = 5.0
    assert abs(x//2 - 2) < eps
    assert abs(x//2.0 - 2.0) < eps
    x = -5.0
    assert abs(x//2 + 3.0) < eps
    assert abs(x//2.0 + 3.0) < eps

def test_floor_div_9_digits():
    # reference: issue 768
    x4: i32
    y4: i32
    y4 = 10
    x4 = 123456789
    assert x4//y4 == 12345678

    x8: i64
    y8: i64
    y8 = 10
    x8 = 123456789
    assert x8//y8 == 12345678


def check():
    test_issue_255()
    main0()
    test_floor_div()
    test_floor_div_9_digits()

check()
