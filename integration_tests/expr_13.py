from lpython import CPtr, empty_c_void_p, i32, Pointer, ccall, Const

@ccall
def deref_array(x: Pointer[CPtr], idx: i32) -> CPtr:
    pass

@ccall
def get_arrays(num_arrays: i32) -> Pointer[CPtr]:
    pass

@ccall
def sum_array(x: CPtr, size: i32) -> i32:
    pass

def sum_array_python(x: Const[CPtr], size: i32) -> i32:
    return sum_array(x, size)

def test_pointer_to_cptr():
    x: Pointer[CPtr] = empty_c_void_p()
    x = get_arrays(2)
    i: i32
    sums: i32[2]
    for i in range(2):
        sums[i] = sum_array_python(deref_array(x, i), 10)

    assert sums[0] == sums[1]

test_pointer_to_cptr()
