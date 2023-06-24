import numpy as np

def get_cpython_version():
    import platform
    return platform.python_version()

def get_int_array_sum(a):
    return np.sum(a)

def get_int_array_product(a):
    return np.prod(a)

def get_float_array_sum(a):
    return np.sum(a)

def get_float_array_product(a):
    return np.prod(a)

def show_array_dot_product(a, b):
    print(a, b)
    print(a @ b)
