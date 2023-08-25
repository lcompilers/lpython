# file: main.py
from lpython import CPtr, i32, dataclass, c_p_pointer, Pointer, empty_c_void_p, p_c_pointer, Array

from numpy import empty, array

@dataclass
class Foo:
     x: i32
     y: i32

def init(foos_ptr: CPtr) -> None:
    foos: Pointer[Array[Foo, :]] = c_p_pointer(foos_ptr, Array[Foo, :], array([1]))
    foos[0] = Foo(3, 2)

def main() -> None:
    foos: Array[Foo, 1] = empty(1, dtype=Foo)
    foos_ptr: CPtr = empty_c_void_p()
    foos[0] = Foo(0, 1)
    p_c_pointer(foos, foos_ptr)
    init(foos_ptr)
    print("foos[0].x = ", foos[0].x)
    print("foos[0].y = ", foos[0].y)
    assert foos[0].x == 3
    assert foos[0].y == 2

main()
