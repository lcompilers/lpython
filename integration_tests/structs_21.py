from ltypes import i32, CPtr, dataclass, c_p_pointer, p_c_pointer, \
         pointer, empty_c_void_p, Pointer, ccallable

@ccallable
@dataclass
class S:
    a: i32

def f(c: CPtr):
    # The following two lines are the actual test: we take an argument `c` of
    # type CPtr, convert to type Pointer[S] and access the member `a`. This
    # tests that the order of initialization is preserved in the generated LLVM
    # or C code.
    p: Pointer[S] = c_p_pointer(c, S)
    A: i32 = p.a

    # We check that we get the expected result:
    print(A)
    assert A == 3

def main():
    s: S = S(3)
    p: CPtr = empty_c_void_p()
    p_c_pointer(pointer(s, S), p)
    f(p)

main()
