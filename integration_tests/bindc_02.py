from ltypes import c_p_pointer, CPtr, pointer, i16, Pointer

queries: CPtr
x: Pointer[i16[:]]
c_p_pointer(queries, x)
print(queries, x)

def f():
    yq: CPtr
    yptr1: Pointer[i16[:]]
    y: i16[2]
    y[0] = 1
    y[1] = 2
    yptr1 = pointer(y)
    print(yptr1[0], yptr1[1])
    print(pointer(y), yptr1)

    c_p_pointer(yq, yptr1)

    print(yq, yptr1)

f()
