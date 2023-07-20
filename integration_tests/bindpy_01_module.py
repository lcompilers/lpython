def get_cpython_version():
    import platform
    return platform.python_version()

def add_ints(a, b, c, d):
    e = a + b + c + d
    return e

def multiply_ints(a, b, c, d):
    e = a * b * c * d
    return e

def add_unsigned_ints(a, b, c, d):
    e = a + b + c + d
    return e

def multiply_unsigned_ints(a, b, c, d):
    e = a * b * c * d
    return e

def add_floats(a, b):
    return a + b

def multiply_floats(a, b):
    return a * b

def get_hello_world(a, b):
    return f"{a} {b}!"

def str_n_times(a, n):
    return a * n
