from ltypes import TypeVar

T = TypeVar('T')

def f(lst: T[:], i: T) -> T:
    lst[0] = i
    return lst[0]

def use_array():
    array: i32[1]
    x: i32
    x = 69
    print(f(array, x))

use_array()