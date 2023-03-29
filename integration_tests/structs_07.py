from lpython import i32, i64, i8, CPtr, sizeof, dataclass, ccall

@dataclass
class A:
    x: i32
    y: i64
    z: i8

@ccall
def cmalloc(bytes: i64) -> CPtr:
    pass

@ccall
def cfree(x: CPtr) -> i32:
    pass

def call_malloc():
   x: CPtr = cmalloc(sizeof(A) * i64(20))
   assert cfree(x) == 1

call_malloc()
