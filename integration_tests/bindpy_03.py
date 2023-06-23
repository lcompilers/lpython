from lpython import i32, i64, f64, pythoncall, Const, TypeVar
from numpy import empty, int32, int64, float64

n = TypeVar("n")
m = TypeVar("m")
p = TypeVar("p")
q = TypeVar("q")
r = TypeVar("r")

@pythoncall(module = "bindpy_03_module")
def get_cpython_version() -> str:
    pass

@pythoncall(module = "bindpy_03_module")
def get_int_array_sum(n: i32, a: i32[:], b: i32[:]) -> i32[n]:
    pass

@pythoncall(module = "bindpy_03_module")
def get_int_array_product(n: i32, a: i32[:], b: i32[:]) -> i32[n]:
    pass

@pythoncall(module = "bindpy_03_module")
def get_float_array_sum(n: i32, m: i32, a: f64[:], b: f64[:]) -> f64[n, m]:
    pass

@pythoncall(module = "bindpy_03_module")
def get_float_array_product(n: i32, m: i32, a: f64[:], b: f64[:]) -> f64[n, m]:
    pass

@pythoncall(module = "bindpy_03_module")
def get_array_dot_product(m: i32, a: i64[:], b: f64[:]) -> f64[m]:
    pass

@pythoncall(module = "bindpy_03_module")
def get_multidim_array_i64(p: i32, q: i32, r: i32) -> i64[p, q, r]:
    pass

# Integers:
def test_array_ints():
    n: Const[i32] = 5
    a: i32[n] = empty([n], dtype=int32)
    b: i32[n] = empty([n], dtype=int32)

    i: i32
    for i in range(n):
        a[i] = i + 10
    for i in range(n):
        b[i] = i + 20

    c: i32[n] = get_int_array_sum(n, a, b)
    print(c)
    for i in range(n):
        assert c[i] == (i + i + 30)


    c = get_int_array_product(n, a, b)
    print(c)
    for i in range(n):
        assert c[i] == ((i + 10) * (i + 20))

# Floats
def test_array_floats():
    n: Const[i32] = 3
    m: Const[i32] = 5
    a: f64[n, m] = empty([n, m], dtype=float64)
    b: f64[n, m] = empty([n, m], dtype=float64)

    i: i32
    j: i32

    for i in range(n):
        for j in range(m):
            a[i, j] = f64((i + 10) * (j + 10))

    for i in range(n):
        for j in range(m):
            b[i, j] = f64((i + 20) * (j + 20))

    c: f64[n, m] = get_float_array_sum(n, m, a, b)
    print(c)
    for i in range(n):
        for j in range(m):
            assert abs(c[i, j] - (f64((i + 10) * (j + 10)) + f64((i + 20) * (j + 20)))) <= 1e-4

    c = get_float_array_product(n, m, a, b)
    print(c)
    for i in range(n):
        for j in range(m):
            assert abs(c[i, j] - (f64((i + 10) * (j + 10)) * f64((i + 20) * (j + 20)))) <= 1e-4

def test_array_broadcast():
    n: Const[i32] = 3
    m: Const[i32] = 5
    a: i64[n] = empty([n], dtype=int64)
    b: f64[n, m] = empty([n, m], dtype=float64)

    i: i32
    j: i32
    for i in range(n):
        a[i] = i64(i + 10)

    for i in range(n):
        for j in range(m):
            b[i, j] = f64((i + 1) * (j + 1))

    c: f64[m] = get_array_dot_product(m, a, b)
    print(c)
    assert abs(c[0] - (68.0)) <= 1e-4
    assert abs(c[1] - (136.0)) <= 1e-4
    assert abs(c[2] - (204.0)) <= 1e-4
    assert abs(c[3] - (272.0)) <= 1e-4
    assert abs(c[4] - (340.0)) <= 1e-4

def test_multidim_array_return_i64():
    p: Const[i32] = 3
    q: Const[i32] = 4
    r: Const[i32] = 5
    a: i64[p, q, r] = empty([p, q, r], dtype=int64)
    a = get_multidim_array_i64(p, q, r)
    print(a)

    i: i32; j: i32; k: i32
    for i in range(p):
        for j in range(q):
            for k in range(r):
                assert a[i, j, k] == i64(i * 2 + j * 3 + k * 4)

def main0():
    print("CPython version: ", get_cpython_version())

    test_array_ints()
    test_array_floats()
    test_array_broadcast()
    test_multidim_array_return_i64()

main0()
