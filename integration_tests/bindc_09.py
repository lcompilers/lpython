from enum import Enum

from lpython import (CPtr, c_p_pointer, p_c_pointer, dataclass, empty_c_void_p,
        pointer, Pointer, i32, ccallable, InOut)

class Value(Enum):
    TEN: i32 = 10
    TWO: i32 = 2
    ONE: i32 = 1
    FIVE: i32 = 5

@dataclass
class Foo:
    value: Value

@ccallable
@dataclass
class FooC:
    value: Value

def bar(foo: InOut[Foo]) -> None:
    foo.value = Value.FIVE

def barc(foo_ptr: CPtr) -> None:
    foo: Pointer[FooC] = c_p_pointer(foo_ptr, FooC)
    foo.value = Value.ONE

def main() -> None:
    foo: Foo = Foo(Value.TEN)
    fooc: FooC = FooC(Value.TWO)
    foo_ptr: CPtr = empty_c_void_p()

    bar(foo)
    print(foo.value, foo.value.name)
# Does not work: https://github.com/lcompilers/lpython/issues/2123
#    assert foo.value == Value.FIVE

    p_c_pointer(pointer(fooc), foo_ptr)
    barc(foo_ptr)
    print(fooc.value)
    assert fooc.value == Value.ONE.value

main()
