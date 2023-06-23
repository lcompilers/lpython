import numpy as np

def get_uint_array_sum(n, a, b):
    return np.add(a, b)

def get_bool_array_or(n, a, b):
    return np.logical_or(a, b)

def get_complex_array_product(n, a, b):
    print(a, b)
    print(np.multiply(a, b))
    return np.multiply(a, b)

def get_2D_uint_array_sum(n, m, a, b):
    return np.add(a, b)

def get_2D_bool_array_and(n, m, a, b):
    return np.logical_and(a, b)

def get_2D_complex_array_sum(n, m, a, b):
    return np.add(a, b)
