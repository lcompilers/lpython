from ltypes import i32, f64

def test_bool():
    a: i32
    a = 34
    assert bool(a)
    a = 0
    assert not bool(a)
    assert bool(-1)
    assert not bool(0)

    f: f64
    f = 0.0
    assert not bool(f)
    f = 1.0
    assert bool(f)
    assert bool(56.7868658)
    assert not bool(0.0)

test_bool()
