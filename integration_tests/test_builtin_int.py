from ltypes import f64, i32, i64

def test_int():
    f: f64
    f = 5.678
    i: i32
    i = 4
    assert int() == 0
    assert int(i) == 4
    i2: i64
    i2 = int(3.0)
    assert i2 == 3
    assert int(5.678) == 5
    assert int(f) == 5
    f = -183745.23
    assert int(-183745.23) == -183745
    assert int(f) == -183745
    assert int(5.5) == 5
    assert int(-5.5) == -5
    assert int(True) == 1
    assert int(False) == 0


def test_bool_to_int():
    b: i32
    b = True - True
    assert b == 0
    b = False - False
    assert b == 0
    b = False - True
    assert b == -1
    b = True - False
    assert b == 1
    b = True + True
    assert b == 2
    b = False + False
    assert b == 0
    b = False + True
    assert b == 1
    b = True + False
    assert b == 1
    b = True + True + True - False
    assert b == 3
    b = True + (True + True) - (False + True)
    assert b == 2


def check_all():
    test_int()
    test_bool_to_int()

check_all()
