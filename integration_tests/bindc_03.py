from ltypes import c_p_pointer, CPtr, pointer, i16, i32, Pointer, ccall, p_c_pointer

@ccall
def g(a: CPtr, value: i32) -> None:
    pass

@ccall
def get_array(size: i32) -> CPtr: # this function can be called from Python but is defined in C code
    pass

@ccallable
def f(q_void: CPtr) -> None: # this function can be called from C code
    i: i32
    el: i32
    q: Pointer[i32[:]]
    c_p_pointer(q_void, q)
    for i in range(10):
        q2: CPtr
        p_c_pointer(pointer(q[i]), q2)
        g(q2, i * i)
        # TODO: Use q[i] directly in the assert.
        el = q[i]
        print(el)
        assert el == i * i

def run():
    a: CPtr
    size: i32
    size = 10
    a = get_array(size)
    f(a)

run()
