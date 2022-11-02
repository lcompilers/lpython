from ltypes import i32, f64, ccall
from numpy import empty, int32, float64

@ccall
def compare_array_element(value1: i32, value2: f64) -> i32:
    pass

def test_arrays():
    array1: i32[40] = empty(40, dtype=int32)
    array2: f64[40] = empty(40, dtype=float64)
    i: i32

    for i in range(40):
        array1[i] = i + 1
        array2[i] = float(2*i + 1)

    is_small: bool = True
    for i in range(40):
        is_small = is_small and bool(compare_array_element(array1[i], array2[i]))

    print(is_small)
    assert is_small == True

test_arrays()
