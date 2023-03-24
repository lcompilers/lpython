from lpython import f64, i32, i64

def test_int():
    f: f64
    f = 5.678
    i: i32
    i = 4
    assert int() == i64(0)
    assert int(i) == i64(4)
    i2: i64
    i2 = int(3.0)
    assert i2 == i64(3)
    assert int(5.678) == i64(5)
    assert int(f) == i64(5)
    f = -183745.23
    assert int(-183745.23) == i64(-183745)
    assert int(f) == i64(-183745)
    assert int(5.5) == i64(5)
    assert int(-5.5) == i64(-5)
    assert int(True) == i64(1)
    assert int(False) == i64(0)


def test_bool_to_int():
    b: i32
    b = i32(True - True)
    assert b == 0
    b = i32(False - False)
    assert b == 0
    b = i32(False - True)
    assert b == -1
    b = i32(True - False)
    assert b == 1
    b = i32(True + True)
    assert b == 2
    b = i32(False + False)
    assert b == 0
    b = i32(False + True)
    assert b == 1
    b = i32(True + False)
    assert b == 1
    b = i32(True + True) + i32(True - False)
    print(b)
    assert b == 3
    b = i32(True) + i32(True + True) - i32(False + True)
    assert b == 2


def check_all():
    test_int()
    test_bool_to_int()

check_all()
