from lpython import f32, f64, i32, i64, i8, i16

def test_abs():
    x: f64
    x = 5.5
    assert abs(x) == 5.5
    x = -5.5
    assert abs(x) == 5.5
    assert abs(5.5) == 5.5
    assert abs(-5.5) == 5.5

    x2: f32
    x2 = -f32(5.5)
    assert abs(x2) == f32(5.5)

    i: i32
    i = -5
    assert abs(i) == 5
    assert abs(-1) == 1

    i2: i64
    i2 = -i64(6)
    assert abs(i2) == i64(6)

    i3: i8
    i3 = -i8(7)
    assert abs(i3) == i8(7)

    i4: i16
    i4 = -i16(8)
    assert abs(i4) == i16(8)

    b: bool
    b = True
    assert abs(b) == 1
    b = False
    assert abs(b) == 0

test_abs()
