from ltypes import CPtr, i64, sizeof, i32, ccall, c_p_pointer, empty_c_void_p, Pointer, pointer
from numpy import empty, int64

@ccall
def _lfortran_malloc(size: i32) -> CPtr:
    pass

def allocate_memory(size: i32) -> tuple[CPtr, CPtr, CPtr, CPtr]:
    array1: CPtr = _lfortran_malloc(size * i32(sizeof(i64)))
    array2: CPtr = _lfortran_malloc(size * i32(sizeof(i64)))
    array3: CPtr = _lfortran_malloc(size * i32(sizeof(i64)))
    array4: CPtr = _lfortran_malloc(size * i32(sizeof(i64)))
    return array1, array2, array3, array4

def sum_arrays(array1: CPtr, array2: CPtr, array3: CPtr, array4: CPtr, size: i32):
    iarray1: Pointer[i64[size]] = c_p_pointer(array1, i64[size])
    iarray2: Pointer[i64[size]] = c_p_pointer(array2, i64[size])
    iarray3: Pointer[i64[size]] = c_p_pointer(array3, i64[size])
    iarray4: Pointer[i64[size]] = c_p_pointer(array4, i64[size])
    sum_array_cptr: CPtr = _lfortran_malloc(size * i32(sizeof(i64)))
    sum_array: Pointer[i64[size]] = c_p_pointer(sum_array_cptr, i64[size])
    i: i32

    for i in range(size):
        iarray1[i] = i64(i)
        iarray2[i] = i64(2 * i)
        iarray3[i] = i64(3 * i)
        iarray4[i] = i64(4 * i)

    for i in range(size):
        sum_array[i] = iarray1[i] + iarray2[i] + iarray3[i] + iarray4[i]

    for i in range(size):
        print(i, sum_array[i])
        assert sum_array[i] == i64(10 * i)

def test_tuple_return():
    a: CPtr = empty_c_void_p()
    b: CPtr = empty_c_void_p()
    c: CPtr = empty_c_void_p()
    d: CPtr = empty_c_void_p()
    a, b, c, d = allocate_memory(50)
    sum_arrays(a, b, c, d, 50)

test_tuple_return()
