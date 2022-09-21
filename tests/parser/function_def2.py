def test_01():
    pass

def test_02(*x: i32):
    pass

def test_03(*x, y, z):
    pass

def test_04(*x, y, z, **args):
    pass

def test_05(*x, **args):
    pass

def test_06(**args):
    pass

def test_07(x: i32, y):
    pass

def test_08(x, *y):
    pass

def test_09(x, *y, z):
    pass

def test_10(x, *y, z, **args: i32):
    pass

def test_11(x, *y, **args):
    pass

def test_12(x, **args):
    pass

def test_13(x: i32 = 1, y: i32 = 1):
    pass

def test_14(x, *y, z, **args: i32) -> i32:
    pass

def test_15(a, /):
    pass

def test_16(a, /, b, c):
    pass

def test_17(a, /, *b):
    pass

def test_18(a:i32, /, *b: i64, c: i32, d: i32):
    pass

def test_19(a, /, *b, c, **d):
    pass

def test_20(a, /, **b):
    pass

def test_21(a, /, *b, **c):
    pass

def test_22(a, /, b, c, *d):
    pass

def test_23(a, /, b, c, **d):
    pass

def test_24(a, /, b, c, *d, **e):
    pass

def test_25(a, /, b, c, *d, e, **f):
    pass

def test_26(*, a):
    pass

def test_27(*, a, **b):
    pass

def test_28(a, *, b):
    pass

def test_29(a, *, b, **c):
    pass

def test_30(a, /, *, b):
    pass

def test_31(a, /, b, *, c):
    pass

def test_32(a, /, *, c, **d):
    pass

def test_33(a, /, b, *, c, **d):
    pass

# Function Calls
test()
test(x, y)
test(x, y = 1, z = '123')
test(100, **x)
test(*x, **y)
test(*x, y)
test(*x, *y)
test(**x, **y)
test(*x, y = 1, z = 2, **a, b = 1)
test(*x, **a, b = "1")
test(x = 1, *y, z = 2, **a, b = "1")
lp.test()
lp.test(x, y)
lp.test(x, y = 1, z = '123')

test()["version"]
test(x, y)["version"]
test(x, *y)[:-1]
test()[1][1]

x["numpystr"]()
x['int']()
x[func](*args, **kwargs)
test[0](*test[1:])
(obj*self._arr.ndim)(*self._arr.shape)
traverse((index, value), visit, result=result, *args, **kwargs)
