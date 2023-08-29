from lpython import i1, i32, u32, f64, c64, pythoncall, Const, TypeVar
from numpy import empty, uint32, complex128

n = TypeVar("n")
m = TypeVar("m")

# Defining the pythoncall decorator functions
@pythoncall(module = "bindpy_04_module")
def get_uint_array_sum(n: i32, a: u32[:], b: u32[:]) -> u32[n]:
    pass

@pythoncall(module = "bindpy_04_module")
def get_bool_array_or(n: i32, a: i1[:], b: i1[:]) -> i1[n]:
    pass

@pythoncall(module = "bindpy_04_module")
def get_complex_array_product(n: i32, a: c64[:], b: c64[:]) -> c64[n]:
    pass

@pythoncall(module = "bindpy_04_module")
def get_2D_uint_array_sum(n: i32, m: i32, a: u32[:,:], b: u32[:,:]) -> u32[n,m]:
    pass

@pythoncall(module = "bindpy_04_module")
def get_2D_bool_array_and(n: i32, m: i32, a: i1[:,:], b: i1[:,:]) -> i1[n,m]:
    pass

@pythoncall(module = "bindpy_04_module")
def get_2D_complex_array_sum(n: i32, m: i32, a: c64[:,:], b: c64[:,:]) -> c64[n,m]:
    pass

# Unsigned Integers
def test_array_uints():
    n: Const[i32] = 5
    a: u32[n] = empty([n], dtype=uint32)
    b: u32[n] = empty([n], dtype=uint32)

    i: i32
    for i in range(n):
        a[i] = u32(i + 10)
        b[i] = u32(i + 20)

    c: u32[n] = get_uint_array_sum(n, a, b)
    print(c)
    for i in range(n):
        assert c[i] == u32(i * 2 + 30)

def test_2D_array_uints():
    n: Const[i32] = 3
    m: Const[i32] = 4
    a: u32[n, m] = empty([n, m], dtype=uint32)
    b: u32[n, m] = empty([n, m], dtype=uint32)

    i: i32
    j: i32
    for i in range(n):
        for j in range(m):
            a[i, j] = u32(i * 10 + j)
            b[i, j] = u32(i * 20 + j)

    c: u32[n, m] = get_2D_uint_array_sum(n, m, a, b)
    print(c)
    for i in range(n):
        for j in range(m):
            assert c[i, j] == u32(i * 30 + 2*j)

# Boolean
def test_array_bools():
    n: Const[i32] = 5
    a: i1[n] = empty([n], dtype=bool)
    b: i1[n] = empty([n], dtype=bool)

    i: i32
    for i in range(n):
        a[i] = bool(i % 2 == 0)
        b[i] = bool(i % 3 == 0)

    c: i1[n] = get_bool_array_or(n, a, b)
    print(c)
    for i in range(n):
        assert c[i] == bool((i % 2 == 0) or (i % 3 == 0))

def test_2D_array_bools():
    n: Const[i32] = 3
    m: Const[i32] = 4
    a: i1[n, m] = empty([n, m], dtype=bool)
    b: i1[n, m] = empty([n, m], dtype=bool)

    i: i32
    j: i32
    for i in range(n):
        for j in range(m):
            a[i, j] = bool(i % 2 == 0)
            b[i, j] = bool(j % 2 == 0)

    c: i1[n, m] = get_2D_bool_array_and(n, m, a, b)
    print(c)
    for i in range(n):
        for j in range(m):
            assert c[i, j] == bool((i % 2 == 0) and (j % 2 == 0))

# Complex
def test_array_complexes():
    n: Const[i32] = 5
    a: c64[n] = empty([n], dtype=complex128)
    b: c64[n] = empty([n], dtype=complex128)

    i: i32
    for i in range(n):
        a[i] = c64(complex(i, i + 10))
        b[i] = c64(complex(i + 1, i + 11))

    c: c64[n] = get_complex_array_product(n, a, b)
    print(c)
    for i in range(n):
        p: f64 = f64(i)
        q: f64 = f64(i + 10)
        r: f64 = f64(i + 1)
        s: f64 = f64(i + 11)
        assert abs(c[i] - c64(complex((p*r - q*s), (p*s + q*r)))) <= 1e-5

def test_2D_array_complexes():
    n: Const[i32] = 3
    m: Const[i32] = 4
    a: c64[n, m] = empty([n, m], dtype=complex128)
    b: c64[n, m] = empty([n, m], dtype=complex128)

    i: i32
    j: i32
    for i in range(n):
        for j in range(m):
            a[i, j] = c64(complex(i, 10*j))
            b[i, j] = c64(complex(i + 1, 10 * (j + 1)))

    c: c64[n, m] = get_2D_complex_array_sum(n, m, a, b)
    print(c)
    for i in range(n):
        for j in range(m):
            assert abs(c[i, j] - c64(complex(2*i + 1, 20*j + 10))) <= 1e-5

def main0():
    test_array_uints()
    test_array_bools()
    test_array_complexes()

    test_2D_array_uints()
    test_2D_array_bools()
    test_2D_array_complexes()

main0()
