from ltypes import sizeof, i64, i32, f32, f64, c32, c64, i16, ccall, CPtr

@ccall
def cmalloc(bytes: i64) -> CPtr:
    pass

@ccall
def cfree(x: CPtr) -> bool:
    pass

@ccall
def fill_carray(arr: CPtr, size: i32):
    pass

@ccall
def sum_carray(arr: CPtr, size: i32) -> i64:
    pass

def test_sizeof():
    xi16: i16 = i16(0)
    xi: i32 = 0
    yi: i64 = i64(0)
    xf: f32 = f32(0.0)
    yf: f64 = 0.0
    xz: c32 = c32(complex(0, 0))
    yz: c64 = complex(0, 0)
    assert sizeof(xi16) == sizeof(i16)
    assert sizeof(xi) == sizeof(i32)
    assert sizeof(yi) == sizeof(i64)
    assert sizeof(xf) == sizeof(f32)
    assert sizeof(yf) == sizeof(f64)
    assert sizeof(xz) == sizeof(c32)
    assert sizeof(yz) == sizeof(c64)


def test_c_array():
    summed_up: i64
    carray: CPtr = cmalloc(sizeof(i64) * i64(100))
    fill_carray(carray, 100)
    summed_up = sum_carray(carray, 100)
    print(summed_up)
    assert summed_up == int(5050)

test_sizeof()
test_c_array()
