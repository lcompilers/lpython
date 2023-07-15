# This test handles actual LPython implementations of functions from the numpy
# module.
from lpython import i32, i64, f32, f64, c32, c64, TypeVar, overload
from numpy import empty, int64

e: f64 = 2.718281828459045
pi: f64 = 3.141592653589793
euler_gamma: f64 = 0.5772156649015329
PZERO: f64 = 0.0
NZERO: f64 = -0.0

eps: f64
eps = 1e-12

n = TypeVar("n")

def zeros(n: i32) -> f64[n]:
    A: f64[n]
    A = empty(n)
    i: i32
    for i in range(n):
        A[i] = 0.0
    return A

def ones(n: i32) -> f64[n]:
    A: f64[n]
    A = empty(n)
    i: i32
    for i in range(n):
        A[i] = 1.0
    return A

def arange(n: i32) -> i64[n]:
    A: i64[n]
    A = empty(n, dtype=int64)
    i: i32
    for i in range(n):
        A[i] = i64(i)
    return A

#: sqrt() as a generic procedure.
#: supported types for argument:
#: i32, i64, f32, f64, bool
@overload
def sqrt(n: i32) -> f64:
    return f64(n)**(1/2)

@overload
def sqrt(n: i64) -> f64:
    return f64(n)**(1/2)

@overload
def sqrt(f: f32) -> f32:
    half: f32
    half = f32(1/2)
    return f**half

@overload
def sqrt(f: f64) -> f64:
    return f**(1/2)

@overload
def sqrt(b: bool) -> f64:
    if b:
        return 1.0
    else:
        return 0.0

#: exp() as a generic procedure.
#: supported types for argument:
#: i32, i64, f32, f64, bool
@overload
def exp(n: i32) -> f64:
    return e**f64(n)

@overload
def exp(n: i64) -> f64:
    return e**f64(n)

@overload
def exp(f: f32) -> f32:
    ef32: f32
    ef32 = f32(e)
    return ef32**f

@overload
def exp(f: f64) -> f64:
    return e**f

@overload
def exp(b: bool) -> f64:
    if b:
        return 2.719
    else:
        return 1.0

#: fabs() as a generic procedure.
#: supported types for argument:
#: i32, i64, f32, f64, bool
@overload
def fabs(n: i32) -> f64:
    if n < 0:
        return -f64(n)
    return f64(n)

@overload
def fabs(n: i64) -> f64:
    if n < i64(0):
        return -f64(n)
    return f64(n)

@overload
def fabs(f: f32) -> f32:
    if f < f32(0.0):
        return -f
    return f

@overload
def fabs(f: f64) -> f64:
    if f < 0.0:
        return -f
    return f

@overload
def fabs(b: bool) -> f64:
    return sqrt(b)

num = TypeVar("num")
def linspace(start: f64, stop: f64, num: i32) -> f64[num]:
    A: f64[num]
    A = empty(num)
    i: i32
    for i in range(num):
        A[i] = start + (stop-start)*f64(i)/f64(num-1)
    return A

#------------------------------
@overload
def sign(x: i32) -> i32:
    if x == 0:
        return 0
    elif x > 0:
        return 1

    return -1

@overload
def sign(x: i64) -> i64:
    result: i64
    if x == i64(0):
        result = i64(0)
    elif x > i64(0):
        result = i64(1)
    else:
        result = -i64(1)
    return result

@overload
def sign(x: f32) -> f32:
    fabsf32: f32
    fabsf32 = fabs(x)
    return f32(x/fabsf32)

@overload
def sign(x: f64) -> f64:
    return x/fabs(x)

#------------------------------
@overload
def real(c: c32) -> f32:
    return c.real

@overload
def real(c: c64) -> f64:
    return c.real

@overload
def real(x: i32) -> i32:
    return x

@overload
def real(x: i64) -> i64:
    return x

@overload
def real(f: f32) -> f32:
    return f

@overload
def real(f: f64) -> f64:
    return f

@overload
def real(b: bool) -> i32:
    if b:
        return 1

    return 0

#------------------------------
@overload
def imag(c: c32) -> f32:
    return f32(c.imag)

@overload
def imag(c: c64) -> f64:
    return c.imag

@overload
def imag(x: i32) -> i32:
    return 0

@overload
def imag(x: i64) -> i64:
    return i64(0)

@overload
def imag(f: f32) -> f32:
    return f32(0.0)

@overload
def imag(f: f64) -> f64:
    return 0.0

@overload
def imag(b: bool) -> i32:
    return 0
#------------------------------

def test_zeros():
    a: f64[4]
    a = zeros(4)
    assert abs(a[0] - 0.0) < eps
    assert abs(a[1] - 0.0) < eps
    assert abs(a[2] - 0.0) < eps
    assert abs(a[3] - 0.0) < eps

def test_ones():
    a: f64[4]
    a = ones(4)
    assert abs(a[0] - 1.0) < eps
    assert abs(a[1] - 1.0) < eps
    assert abs(a[2] - 1.0) < eps
    assert abs(a[3] - 1.0) < eps

def test_arange():
    a: i64[4]
    a = arange(4)
    assert a[0] == i64(0)
    assert a[1] == i64(1)
    assert a[2] == i64(2)
    assert a[3] == i64(3)

def test_sqrt():
    a: f64
    a2: f64
    a = sqrt(2)
    a2 = sqrt(5.6)
    assert abs(a - 1.4142135623730951) < eps
    assert abs(a2 - 2.3664319132398464) < eps
    assert abs(sqrt(False) - 0.0) < eps

    i: i64
    i = i64(4)
    a = sqrt(i)
    assert abs(f64(a) - 2.0) < eps

    f: f32
    f = f32(4.0)
    assert abs(f64(sqrt(f)) - 2.0) < eps

def test_exp():
    a: f64
    a = exp(6)
    a2: f64
    a2 = exp(5.6)

    assert abs(a - 403.4287934927351) < eps
    assert abs(a2 - 270.42640742615254) < eps
    assert abs(exp(True) - 2.719) < eps

    i: i64
    i = i64(4)
    a = exp(i)
    assert abs(a - 54.598150033144236) < eps

    f: f32
    f = -f32(4.0)
    print(exp(f))

def test_fabs():
    a: f64
    a = fabs(-3.7)
    a2: f64
    a2 = fabs(-3)
    assert abs(a - 3.7) < eps
    assert abs(a2 - 3.0) < eps
    assert abs(fabs(True) - 1.0) < eps

    i: i64
    i = -i64(4)
    a = fabs(i)
    assert abs(a - 4.0) < eps

    f: f32
    f = -f32(4.0)
    assert abs(f64(fabs(f)) - 4.0) < eps

def test_linspace():
    a: f64[4]
    a = linspace(1., 7., 4)
    assert abs(a[0] - 1.0) < eps
    assert abs(a[1] - 3.0) < eps
    assert abs(a[2] - 5.0) < eps
    assert abs(a[3] - 7.0) < eps

def test_sign():
    a: i32
    a = sign(-3)
    assert a == -1
    assert sign(0) == 0

    f: f32
    f = -f32(3.0)
    assert sign(f) == -f32(1.0)
    f = f32(235.4142135623730951)
    assert sign(f) == f32(1.0)

    a2: i64
    a2 = sign(i64(3))
    assert a2 == i64(1)

    f2: f64
    f2 = -3.0
    assert sign(f2) == -1.0

def test_real():
    c: c32
    c = c32(4) + c32(3j)
    assert abs(f64(real(c)) - 4.0) < eps

    c2: c64
    c2 = complex(5, -6)
    assert abs(real(c2) - 5.0) < eps

    i: i32
    i = 4
    assert real(i) == 4

    i2: i64
    i2 = -i64(4)
    assert real(i2) == -i64(4)

    f: f64
    f = 534.6475
    assert abs(real(f) - 534.6475) < eps
    assert real(True) == 1

def test_imag():
    c: c32
    c = c32(4) + c32(3j)
    assert abs(f64(imag(c)) - 3.0) < eps

    c2: c64
    c2 = complex(5, -6)
    assert abs(f64(imag(c2)) - -6.0) < eps

    i: i32
    i = 4
    assert imag(i) == 0

    i2: i64
    i2 = -i64(4)
    assert imag(i2) == i64(0)

    f: f64
    f = 534.6475
    assert abs(imag(f) - 0.0) < eps
    assert imag(True) == 0

def check():
    test_zeros()
    test_ones()
    test_arange()
    test_sqrt()
    test_exp()
    test_fabs()
    test_linspace()
    test_sign()
    test_real()
    test_imag()

check()
