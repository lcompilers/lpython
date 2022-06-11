from ltypes import c_p_pointer, CPtr, pointer, i16

queries: CPtr
x: Pointer[i16]
c_p_pointer(queries, x)
print(queries, x)
