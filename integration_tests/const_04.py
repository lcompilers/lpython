from ltypes import i32, i16, Const
from numpy import empty, int16

def sum_const_array(array: Const[i16[:]], size: i32) -> i32:
    i: i32
    array_sum: i32 = 0
    for i in range(size):
        array_sum += array[i]
    return array_sum

def test_const_array():
    arr: i16[4] = empty(4, dtype=int16)
    i: i32
    for i in range(4):
        arr[i] = i
    print(sum_const_array(arr))

test_const_array()
