from lpython import (CPtr, sizeof, ccall, c_p_pointer, empty_c_void_p,
        i64, i32, i16, i8,
        u64, u32, u16, u8,
        Pointer, pointer, u16)
from numpy import array

@ccall
def _lfortran_malloc(size: i32) -> CPtr:
    pass

def allocate_memory(size: i32) -> tuple[CPtr, CPtr, CPtr, CPtr, \
        CPtr, CPtr, CPtr, CPtr]:
    array1: CPtr = _lfortran_malloc(size * i32(sizeof(i8)))
    array2: CPtr = _lfortran_malloc(size * i32(sizeof(i16)))
    array3: CPtr = _lfortran_malloc(size * i32(sizeof(i32)))
    array4: CPtr = _lfortran_malloc(size * i32(sizeof(i64)))
    array5: CPtr = _lfortran_malloc(size * i32(sizeof(u8)))
    array6: CPtr = _lfortran_malloc(size * i32(sizeof(u16)))
    array7: CPtr = _lfortran_malloc(size * i32(sizeof(u32)))
    array8: CPtr = _lfortran_malloc(size * i32(sizeof(u64)))
    return array1, array2, array3, array4, array5, array6, array7, array8

def sum_arrays(array1: CPtr, array2: CPtr, array3: CPtr, array4: CPtr, \
        array5: CPtr, array6: CPtr, array7: CPtr, array8: CPtr, size: i32):
    iarray1: Pointer[i8[:]] = c_p_pointer(array1, i8[:], array([size]))
    iarray2: Pointer[i16[:]] = c_p_pointer(array2, i16[:], array([size]))
    iarray3: Pointer[i32[:]] = c_p_pointer(array3, i32[:], array([size]))
    iarray4: Pointer[i64[:]] = c_p_pointer(array4, i64[:], array([size]))
    iarray5: Pointer[u8[:]] = c_p_pointer(array5, u8[:], array([size]))
    iarray6: Pointer[u16[:]] = c_p_pointer(array6, u16[:], array([size]))
    iarray7: Pointer[u32[:]] = c_p_pointer(array7, u32[:], array([size]))
    iarray8: Pointer[u64[:]] = c_p_pointer(array8, u64[:], array([size]))
    sum_array_cptr: CPtr = _lfortran_malloc(size * i32(sizeof(i64)))
    sum_array: Pointer[i64[:]] = c_p_pointer(sum_array_cptr, i64[:], array([size]))
    i: i32

    for i in range(size):
        iarray1[i] = i8(i)
        iarray2[i] = i16(2 * i)
        iarray3[i] = i32(3 * i)
        iarray4[i] = i64(4 * i)
        iarray5[i] = u8(i)
        iarray6[i] = u16(6 * i)
        iarray7[i] = u32(7 * i)
        iarray8[i] = u64(8 * i)

    for i in range(size):
        sum_array[i] = i64(iarray1[i]) + i64(iarray2[i]) + i64(iarray3[i]) \
                + iarray4[i] + i64(iarray5[i]) + i64(iarray6[i]) \
                + i64(iarray7[i]) + i64(iarray8[i])

    for i in range(size):
        print(i, sum_array[i])
        assert sum_array[i] == i64(32 * i)

def test_tuple_return():
    a: CPtr = empty_c_void_p()
    b: CPtr = empty_c_void_p()
    c: CPtr = empty_c_void_p()
    d: CPtr = empty_c_void_p()
    e: CPtr = empty_c_void_p()
    f: CPtr = empty_c_void_p()
    g: CPtr = empty_c_void_p()
    h: CPtr = empty_c_void_p()
    a, b, c, d, e, f, g, h = allocate_memory(50)
    sum_arrays(a, b, c, d, e, f, g, h, 50)

test_tuple_return()
