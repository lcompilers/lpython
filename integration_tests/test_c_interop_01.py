from ltypes import ccall, f32, f64, i32, i64
#from math import pi

@ccall
def _lcompilers_dsin(x: f64) -> f64:
    pass

@ccall
def _lcompilers_ssin(x: f32) -> f32:
    pass

@ccall
def _lcompilers_bgt32(i: i32, j: i32) -> i32:
    pass

@ccall
def _lcompilers_bgt64(i: i64, j: i64) -> i64:
    pass

@ccall
def _lcompilers_random_number(n: i64, v: f64[:]):
    pass

def test_c_callbacks():
    pi: f64 = 3.141592653589793238462643383279502884197
    assert abs(_lcompilers_dsin(pi) - 0) < 1e-12
    assert abs(_lcompilers_dsin(pi/2) - 1) < 1e-12
    #assert abs(_lcompilers_ssin(pi) - 0) < 1e-6
    #assert abs(_lcompilers_ssin(pi/2) - 1) < 1e-6

    assert _lcompilers_bgt32(3, 4) == 0
    assert _lcompilers_bgt32(4, 3) == 1
    #assert _lcompilers_bgt64(3, 4) == 0
    #assert _lcompilers_bgt64(4, 3) == 1

test_c_callbacks()
