def test_boz():
    b: str
    b = bin(5)
    b = bin(64)
    b = bin(-534)
    b = oct(8)
    b = oct(56)
    b = oct(-534)
    b = hex(42)
    b = hex(12648430)
    b = hex(-534)


def test_ord_chr():
    s: str
    a: i32
    a = ord('5')
    s = chr(43)


def test_abs():
    a: i32
    a = abs(5)
    a = abs(-500)
    a = abs(False)
    a = abs(True)
    b: f32
    b = abs(3.45)
    b = abs(-5346.34)
    b = abs(complex(3.45, 5.6))


def test_len():
    a: i32
    a = len('')
    a = len('test')
    a = len("this is a test")
    a = len((1, 2, 3))
    a = len((("c", "b", 3.4), ("c", 3, 5.6)))
    a = len([1, 2, 3])
    a = len([[-4, -5, -6], [-1, -2, -3]])
    a = len({1, 2, 3})
    a = len({1: "c", 2: "b", 3: "c"})
    l: list[i32]
    l = [1, 2, 3, 4]
    a = len(l)
    l.append(5)
    a = len(l)


def test_bool():
    a: bool
    a = bool(0)
    a = bool(-1)
    a = bool('')
    a = bool(complex(0, 0))
    assert a == False
    a = bool('t')
    a = bool(2.3)
    assert a == True


def test_str():
    s: str
    s = str()
    s = str(5)
    s = str(-4)
    # TODO: This test had hidden failure and was noticed during type
    # checking.
    # s = str(5.6)
    s = str(True)
    s = str(False)
    s = str("5346")


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
    a = int()
    a = int(4.56)
    a = int(5)
    a = int(-5.00001)
    a = int(True)
    a = int(False)
    a = int("5346")


def test_float():
    a: f64
    a = float()
    a = float(4.56)
    a = float(5)
    a = float(-1)
    a = float(True)
    a = float(False)
    # commented tests should work
    # a = float("5346")
    # a = float("423.534")


def test_divmod():
    a: tuple[i32, i32]
    a = divmod(9, 3)
    a = divmod(9, -3)
    a = divmod(3, 3)
    a = divmod(4, 5)
    a = divmod(0, 5)
