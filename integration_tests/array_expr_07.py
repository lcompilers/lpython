from lpython import c32, c64, f32
from numpy import complex64, complex128, array

def g():
    a32: c32[4] = array([127, -127, 3, 111], dtype=complex64)
    a64: c64[4] = array([127, -127, 3, 111], dtype=complex128)

    print(a32)
    print(a64)

    assert (abs(a32[0] - c32(127)) <= f32(1e-5))
    assert (abs(a32[1] - c32(-127)) <= f32(1e-5))
    assert (abs(a32[2] - c32(3)) <= f32(1e-5))
    assert (abs(a32[3] - c32(111)) <= f32(1e-5))

    assert (abs(a64[0] - c64(127)) <= 1e-5)
    assert (abs(a64[1] - c64(-127)) <= 1e-5)
    assert (abs(a64[2] - c64(3)) <= 1e-5)
    assert (abs(a64[3] - c64(111)) <= 1e-5)

g()
