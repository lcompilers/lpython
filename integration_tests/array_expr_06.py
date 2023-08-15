from lpython import f32, f64
from numpy import float32, float64, array

def g():
    a32: f32[4] = array([127, -127, 3, 111], dtype=float32)
    a64: f64[4] = array([127, -127, 3, 111], dtype=float64)

    print(a32)
    print(a64)

    assert (abs(a32[0] - f32(127)) <= f32(1e-5))
    assert (abs(a32[1] - f32(-127)) <= f32(1e-5))
    assert (abs(a32[2] - f32(3)) <= f32(1e-5))
    assert (abs(a32[3] - f32(111)) <= f32(1e-5))

    assert (abs(a64[0] - f64(127)) <= 1e-5)
    assert (abs(a64[1] - f64(-127)) <= 1e-5)
    assert (abs(a64[2] - f64(3)) <= 1e-5)
    assert (abs(a64[3] - f64(111)) <= 1e-5)

g()
