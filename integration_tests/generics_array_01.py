from ltypes import TypeVar

T = TypeVar('T')

def f(lst: T[:], i: T) -> T:
    lst[0] = i
    return lst[0]

def use_array():
    array_i: i32[1]
    i: i32
    i = 69
    array_r: f32[1]
    r: f32
    r = 69.0
    print(f(array_i, i))
    print(f(array_r, r))

use_array()