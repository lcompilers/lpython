from ltypes import f64

def test_round():
    f: f64
    f = 5.678
    print(round(f))
    f = -183745.23
    print(round(f))
    f = 44.34
    print(round(f))
    f = 0.5
    print(round(f))
    f = -50.5
    print(round(f))
    f = 1.5
    print(round(f))
    assert round(13.001) == 13
    assert round(-40.49999) == -40
    assert round(0.5) == 0
    assert round(-0.5) == 0
    assert round(1.5) == 2
    assert round(50.5) == 50
    assert round(56.78) == 57


test_round()
