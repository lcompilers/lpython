from lpython import i32, f64, pythoncall, Const
from numpy import empty, int32, float64

@pythoncall(module = "bindpy_02_module")
def get_cpython_version() -> str:
    pass

@pythoncall(module = "bindpy_02_module")
def get_int_array_sum(a: i32[:]) -> i32:
    pass

@pythoncall(module = "bindpy_02_module")
def get_int_array_product(a: i32[:]) -> i32:
    pass

@pythoncall(module = "bindpy_02_module")
def get_float_array_sum(a: f64[:]) -> f64:
    pass

@pythoncall(module = "bindpy_02_module")
def get_float_array_product(a: f64[:]) -> f64:
    pass

@pythoncall(module = "bindpy_02_module")
def show_array_dot_product(a: i32[:], b: f64[:]):
    pass

# Integers:
def test_array_ints():
    n: Const[i32] = 5
    a: i32[n] = empty([n], dtype=int32)

    i: i32
    for i in range(n):
        a[i] = i + 10

    assert get_int_array_sum(a) == i32(60)
    assert get_int_array_product(a) == i32(240240)

# Floats
def test_array_floats():
    n: Const[i32] = 3
    m: Const[i32] = 5
    b: f64[n, m] = empty([n, m], dtype=float64)

    i: i32
    j: i32

    for i in range(n):
        for j in range(m):
            b[i, j] = f64((i + 1) * (j + 1))

    assert abs(get_float_array_sum(b) - (90.000000)) <= 1e-4
    assert abs(get_float_array_product(b) - (13436928000.000000)) <= 1e-4

def test_array_broadcast():
    n: Const[i32] = 3
    m: Const[i32] = 5
    a: i32[n] = empty([n], dtype=int32)
    b: f64[n, m] = empty([n, m], dtype=float64)

    i: i32
    j: i32
    for i in range(n):
        a[i] = i + 10

    for i in range(n):
        for j in range(m):
            b[i, j] = f64((i + 1) * (j + 1))

    show_array_dot_product(a, b)

def main0():
    print("CPython version: ", get_cpython_version())

    test_array_ints()
    test_array_floats()
    test_array_broadcast()

main0()
