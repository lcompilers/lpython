import numpy as np

def get_cpython_version():
    import platform
    return platform.python_version()

def get_int_array_sum(n, a, b):
    return np.add(a, b)

def get_int_array_product(n, a, b):
    return np.multiply(a, b)

def get_float_array_sum(n, m, a, b):
    return np.add(a, b)

def get_float_array_product(n, m, a, b):
    return np.multiply(a, b)

def get_array_dot_product(m, a, b):
    print(a, b)
    c = a @ b
    print(c)
    return c

def get_multidim_array_i64(p, q, r):
    a = np.empty([p, q, r], dtype = np.int64)
    for i in range(p):
        for j in range(q):
            for k in range(r):
                a[i, j, k] = i * 2 + j * 3 + k * 4
    return a
