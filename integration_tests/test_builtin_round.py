from ltypes import f64

def test_round():
    f: f64
    f = 5.678
    print(round(f))
    assert round(f) == 6
    f = -183745.23
    print(round(f))
    assert round(f) == -183745
    f = 44.34
    print(round(f))
    assert round(f) == 44
    assert round(13.001) == 13
    assert round(-40.49999) == -40


test_round()
