from lpython import i32, i64, u32, u64, f32, f64, pythoncall

@pythoncall(module = "bindpy_01_module")
def add_ints(a: i32, b: i32, c: i32, d: i32) -> i64:
    pass

@pythoncall(module = "bindpy_01_module")
def multiply_ints(a: i32, b: i32, c: i32, d: i32) -> i64:
    pass

@pythoncall(module = "bindpy_01_module")
def add_unsigned_ints(a: u32, b: u32, c: u32, d: u32) -> u64:
    pass

@pythoncall(module = "bindpy_01_module")
def multiply_unsigned_ints(a: u32, b: u32, c: u32, d: u32) -> u64:
    pass

@pythoncall(module = "bindpy_01_module")
def add_floats(a: f32, b: f64) -> f64:
    pass

@pythoncall(module = "bindpy_01_module")
def multiply_floats(a: f32, b: f64) -> f64:
    pass

@pythoncall(module = "bindpy_01_module")
def get_hello_world(a: str, b: str) -> str:
    pass

@pythoncall(module = "bindpy_01_module")
def str_n_times(a: str, n: i32) -> str:
    pass

@pythoncall(module = "bindpy_01_module")
def get_cpython_version() -> str:
    pass

# Integers:
def test_ints():
    i: i32
    j: i32
    k: i32
    l: i32
    i = -5
    j = 24
    k = 20
    l = 92

    assert add_ints(i, j, k, l) == i64(131)
    assert multiply_ints(i, j, k, l) == i64(-220800)

# Unsigned Integers:
def test_unsigned_ints():
    i: u32
    j: u32
    k: u32
    l: u32
    i = u32(5)
    j = u32(24)
    k = u32(20)
    l = u32(92)

    assert add_unsigned_ints(i, j, k, l) == u64(141)
    assert multiply_unsigned_ints(i, j, k, l) == u64(220800)

# Floats
def test_floats():
    a: f32
    b: f64
    a = f32(3.14)
    b = -100.00

    assert abs(add_floats(a, b) - (-96.86)) <= 1e-4
    assert abs(multiply_floats(a, b) - (-314.0)) <= 1e-4

# Strings
def test_strings():
    a: str
    b: str
    c: str
    i: i32
    a = "hello"
    b = "world"
    i = 3

    assert get_hello_world(a, b) == "hello world!"
    assert str_n_times(a, i) == "hellohellohello"
    assert get_hello_world(str_n_times(a, i), b) == "hellohellohello world!"

def main0():
    print("CPython version: ", get_cpython_version())

    test_ints()
    test_floats()
    test_strings()


main0()
