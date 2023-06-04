def init_X(m:i32, n: i32, b: CPtr, m: i32) -> None:
    B: Pointer[i16[m*n]] = c_p_pointer(b, i16[m*n])
    i: i32
    j: i32
    for i in range(m):
        for j in range(n):
            B[i * n + j] = i16((i + j) % m)
