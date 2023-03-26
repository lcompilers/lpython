from lpython import i8, i16, i32, i64, f32, f64

def test_round():
    f: f64
    f = 5.678
    assert round(f) == 6
    f = -183745.23
    assert round(f) == -183745
    f = 44.34
    assert round(f) == 44
    f = 0.5
    assert round(f) == 0
    f = -50.5
    assert round(f) == -50
    f = 1.5
    assert round(f) == 2
    assert round(13.001) == 13
    assert round(-40.49999) == -40
    assert round(0.5) == 0
    assert round(-0.5) == 0
    assert round(1.5) == 2
    assert round(50.5) == 50
    assert round(56.78) == 57

    f2: f32
    f2 = f32(5.678)
    assert round(f2) == 6

    i: i32
    i = -5
    assert round(i) == -5
    assert round(4) == 4

    i2: i8
    i2 = i8(7)
    assert round(i2) == i8(7)

    i3: i16
    i3 = i16(-8)
    assert round(i3) == i16(-8)

    i4: i64
    i4 = i64(0)
    assert round(i4) == i64(0)

    b: bool
    b = True
    assert round(b) == 1
    b = False
    assert round(b) == 0
    assert round(False) == 0

test_round()
