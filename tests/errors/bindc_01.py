from lpython import i32, CPtr, ccall

@ccall
def cptr_arg(arg1: CPtr):
    pass

def test_call_cptr():
    int32obj: i32
    cptr_arg(int32obj)

test_call_cptr()
