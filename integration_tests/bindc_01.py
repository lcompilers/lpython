from ltypes import c_p_pointer, CPtr, i16, Pointer

queries: CPtr
x: Pointer[i16] = c_p_pointer(queries, i16)
print(queries, x)
