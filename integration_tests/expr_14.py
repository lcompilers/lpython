from ltypes import i32, i64, f32, f64, c32, c64

def test_divide():
    a1: i32; a2: i32; a3: f64;
    b1: i64; b2: i64; b3: f64;
    c1: f32; c2: f32; c3: f32;
    d1: f64; d2: f64; d3: f64;
    e1: c32; e2: c32; e3: c32;
    f1: c64; f2: c64; f3: c64;

    a1 = 1
    a2 = 9
    a3 = a2/a1
    assert abs(a3 - 9.0) <= 1e-12

    b1 = i64(2)
    b2 = i64(10)
    b3 = b2/b1
    assert abs(b3 - 5.0) <= 1e-12

    c1 = f32(3.0)
    c2 = f32(11.0)
    c3 = c2/c1
    assert abs(c3 - f32(3.666666)) <= f32(1e-6)

    d1 = 4.0
    d2 = 12.0
    d3 = d2/d1
    assert abs(d3 - 3.0) <= 1e-12

    e1 = c32(5) + c32(6j)
    e2 = c32(13) + c32(14j)
    e3 = e2/e1
    assert abs(e3 - c32(2.442622950819672) + c32(0.13114754098360643j)) <= f32(1e-6)

    f1 = c64(7) + 8j
    f2 = c64(15) + 16j
    f3 = f2/f1
    assert abs(f3 - c64(2.061946902654867) + 0.07079646017699115j) <= 1e-6

test_divide()
