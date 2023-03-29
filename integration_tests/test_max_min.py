from lpython import i32, f64

def test_max_int():
    a: i32 = 1
    b: i32 = 2
    c: i32 = 3
    assert max(a, b) == b
    assert max(a, b, c) == c
    assert max(1, 2, 3) == 3
    assert max(1, 6) == 6

def test_max_float():
    d: f64 = 23.233
    e: f64 = 23.2233
    f: f64 = 21.23
    assert max(d, e, f) == d
    assert max(e, f) == e

def test_min_int():
    a: i32 = 1
    b: i32 = 2
    c: i32 = 3
    assert min(a, b) == a
    assert min(a, b, c) == a
    assert min(1, 2, 3) == 1
    assert min(1, 6) == 1

def test_min_float():
    d: f64 = 23.233
    e: f64 = 23.2233
    f: f64 = 21.23
    assert min(d, e, f) == f
    assert min(e, f) == f

def check():
    test_max_int()
    test_max_float()
    test_min_int()
    test_min_float()

check()
