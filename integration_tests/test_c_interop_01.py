from ltypes import ccall, f64, f32
from math import pi

@ccall
def _lfortran_dsin(x: f64) -> f64:
    pass

@ccall
def _lfortran_ssin(x: f32) -> f32:
    pass

assert abs(_lfortran_dsin(pi) - 0) < 1e-12
assert abs(_lfortran_dsin(pi/2) - 1) < 1e-12
assert abs(_lfortran_ssin(pi) - 0) < 1e-6
assert abs(_lfortran_ssin(pi/2) - 1) < 1e-6
