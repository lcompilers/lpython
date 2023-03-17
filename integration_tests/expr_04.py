from ltypes import i32, f32, f64, i64
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
    x = f32(5.0)
    assert f64(abs(x//f32(2) - f32(2))) < eps
    assert f64(abs(x//f32(2.0) - f32(2.0))) < eps
    x = -f32(5.0)
    assert f64(abs(x//f32(2) + f32(3.0))) < eps
    assert f64(abs(x//f32(2.0) + f32(3.0))) < eps

def test_floor_div_9_digits():
    # reference: issue 768
    x4: i32
    y4: i32
    y4 = 10
    x4 = 123456789
    assert x4//y4 == 12345678

    x8: i64
    y8: i64
    y8 = i64(10)
    x8 = i64(123456789)
    assert x8//y8 == i64(12345678)

def test_issue_1586():
    x4: i16
    y4: i16
    y4 = i16(10)
    x4 = i16(12345)
    assert x4//y4 == i16(1234)

    a4: i8
    b4: i8
    a4 = i8(10)
    b4 = i8(123)
    assert b4//a4 == i8(12)

def check():
    test_issue_255()
    main0()
    test_floor_div()
    test_floor_div_9_digits()

check()
