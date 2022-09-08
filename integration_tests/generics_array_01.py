from ltypes import TypeVar
from numpy import empty

T = TypeVar('T')

def f(lst: T[:], i: T) -> T:
    lst[0] = i
    return lst[0]

def use_array():
    array: i32[1]
    array = empty(1)
    x: i32
    x = 69
    print(f(array, x))

use_array()