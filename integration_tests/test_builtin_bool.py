from lpython import i8, i16, i32, i64, f32, f64, c32, c64

def test_bool():
    a: i32
    a = 34
    assert bool(a)
    a = 0
    assert not bool(a)
    assert bool(-1)
    assert not bool(0)

    a2: i64
    a2 = i64(34)
    assert bool(a2)

    a3: i8
    a3 = i8(34)
    assert bool(a3)

    a4: i16
    a4 = -i16(1)
    assert bool(a4)

    f: f64
    f = 0.0
    assert not bool(f)
    f = 1.0
    assert bool(f)
    assert bool(56.7868658)
    assert not bool(0.0)

    f2: f32
    f2 = -f32(235.6)
    assert bool(f2)

    f2 = f32(0.0000534)
    assert bool(f2)

    s: str
    s = ""
    assert not bool(s)
    s = "str"
    assert bool(s)
    assert not bool('')
    assert bool("str")

    b: bool
    b = True
    assert bool(b)
    b = False
    assert not bool(b)
    assert bool(True)
    assert not bool(False)

    c: c32
    c = c32(complex(2, 3))
    assert bool(c)
    c = c32(complex(0, 0))
    assert not bool(c)
    assert not bool(c64(0) + 0j)

    c1: c64
    c1 = complex(0, 0.100202)
    assert bool(c1)
    assert not bool(complex(0, 0))
    assert bool(c64(3) + 5j)


test_bool()
