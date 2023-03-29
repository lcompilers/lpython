from lpython import c_p_pointer, CPtr, i16, Pointer, empty_c_void_p

queries: CPtr = empty_c_void_p()
x: Pointer[i16] = c_p_pointer(queries, i16)
print(queries, x)
