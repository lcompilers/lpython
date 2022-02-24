from ltypes import ctypes, f64
from math import pi

@ctypes
def _lfortran_dsin(x: f64) -> f64:
    pass

assert abs(_lfortran_dsin(pi) - 0) < 1e-12
assert abs(_lfortran_dsin(pi/2) - 1) < 1e-12
