from lpython import i32, CPtr, ccall, dataclass

@dataclass
class Struct:
    cptr_member: CPtr

def test_call_cptr():
    int32obj: i32
    s: Struct = Struct(int32obj)

test_call_cptr()
