from ltypes import i32, i64, f32, f64

def test_bool():
    a: i32
    a = 34
    assert bool(a)
    a = 0
    assert not bool(a)
    assert bool(-1)
    assert not bool(0)

    a2: i64
    a2 = 34
    assert bool(a2)

    f: f64
    f = 0.0
    assert not bool(f)
    f = 1.0
    assert bool(f)
    assert bool(56.7868658)
    assert not bool(0.0)

    f2: f32
    f2 = -235.6
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

test_bool()
