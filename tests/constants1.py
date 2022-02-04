def test_boz():
    b: str
    b = bin(5)
    b = bin(64)
    b = oct(8)
    b = oct(56)
    b = hex(42)
    b = hex(12648430)


def test_ord_chr():
    s: str
    a: i32
    a = ord('5')
    s = chr(43)


def test_abs():
    a: i32
    a = abs(False)
    a = abs(True)
    b: f32
    b = abs(3.45)
    b = abs(complex(3.45, 5.6))


def test_len():
    a: i32
    a = len('test')
    a = len("this is a test")


def test_bool():
    a: bool
    a = bool(0)
    a = bool('')
    a = bool(complex(0, 0))
    assert a == False
    a = bool('t')
    a = bool(2.3)
    assert a == True


def test_callable():
    a: bool
    b: i32
    b = 2
    a = callable(test_len)
    assert a == True
    a = callable(b)
    assert a == False
    a = callable('c')
    assert a == False


def test_int():
    a: i64
    a = int(4.56)
    a = int(5)
    a = int(True)
    a = int(False)
    a = int("5346")
    # a = int(complex(3.45, 5.6)) -> semantic error


def test_float():
    a: f64
    a = float(4.56)
    a = float(5)
    a = float(True)
    a = float(False)
    a = float("5346")
    a = float("423.534")
    # a = float(complex(3.45, 5.6)) -> semantic error
