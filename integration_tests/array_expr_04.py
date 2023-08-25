from lpython import i8, i16, i32, i64
from numpy import int8, int16, int32, int64, array

def g():
    a8: i8[4] = array([127, -127, 3, 111], dtype=int8)
    a16: i16[4] = array([127, -127, 3, 111], dtype=int16)
    a32: i32[4] = array([127, -127, 3, 111], dtype=int32)
    a64: i64[4] = array([127, -127, 3, 111], dtype=int64)

    print(a8)
    print(a16)
    print(a32)
    print(a64)

    assert (a8[0] == i8(127))
    assert (a8[1] == i8(-127))
    assert (a8[2] == i8(3))
    assert (a8[3] == i8(111))

    assert (a16[0] == i16(127))
    assert (a16[1] == i16(-127))
    assert (a16[2] == i16(3))
    assert (a16[3] == i16(111))

    assert (a32[0] == i32(127))
    assert (a32[1] == i32(-127))
    assert (a32[2] == i32(3))
    assert (a32[3] == i32(111))

    assert (a64[0] == i64(127))
    assert (a64[1] == i64(-127))
    assert (a64[2] == i64(3))
    assert (a64[3] == i64(111))

g()
