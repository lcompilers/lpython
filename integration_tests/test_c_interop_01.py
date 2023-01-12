from ltypes import ccall, f32, f64, i32, i64
#from math import pi

@ccall
def _lfortran_dsin(x: f64) -> f64:
    pass

@ccall
def _lfortran_ssin(x: f32) -> f32:
    pass

@ccall
def _lfortran_bgt32(i: i32, j: i32) -> i32:
    pass

@ccall
def _lfortran_bgt64(i: i64, j: i64) -> i32:
    pass

#@ccall
#def _lfortran_random_number(n: i64, v: f64[:]):
#    pass

def test_c_callbacks():
    pi: f64 = 3.141592653589793238462643383279502884197
    assert abs(_lfortran_dsin(pi) - 0.0) < 1e-12
    assert abs(_lfortran_dsin(pi/2) - 1.0) < 1e-12
    assert abs(_lfortran_ssin(f32(pi)) - f32(0.0)) < f32(1e-6)
    assert abs(_lfortran_ssin(f32(pi/2.0)) - f32(1.0)) < f32(1e-6)

    assert _lfortran_bgt32(3, 4) == 0
    assert _lfortran_bgt32(4, 3) == 1
    assert _lfortran_bgt64(i64(3), i64(4)) == 0
    assert _lfortran_bgt64(i64(4), i64(3)) == 1

test_c_callbacks()
