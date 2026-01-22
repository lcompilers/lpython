from numpy import empty, uint16
from lpython import Annotation, u16, SIMD, L1, i32, ccall, i16, i64, CPtr, TypeVar


n = TypeVar("n")
m = TypeVar("m")
l = TypeVar("l")

def matmul_test(l: i32, n: i32, m: i32, M1: i32, M2: i32, V: i32,
                A: u16[n, m], B: u16[m, l], C: u16[n, l]):
    L1cache: Annotation[u16[M2 + 1, V], L1] = empty((M2 + 1, V), dtype=uint16)
    B_vr: Annotation[u16[V], SIMD] = empty((V,), dtype=uint16)
    C_vr: Annotation[u16[V], SIMD] = empty((V,), dtype=uint16)
    T_vr: Annotation[u16[V], SIMD] = empty((V,), dtype=uint16)
    i: i32; j: i32; jj: i32; ii: i32; kk: i32; k: i32; A_ik: u16

    L1_B_index: i32 = 0
    L1_C_base: i32 = 1

    # I think in the APU code, M1=1, so the loops over kk and k get fused
    # Closer to the APU code

    # Wrong value on purpose to check that all entries will be overriden
    C[:,:] = u16(1)
    for jj in range(0, l, V):
        for ii in range(0, n, M2):
            for i in range(M2):
                C_vr[:] = u16(0)
                L1cache[L1_C_base+i,:] = C_vr[:]
            for kk in range(0, m, M1):
                for k in range(M1):
                    L1cache[L1_B_index,:] = B[kk+k, jj:jj+V]
                    B_vr[:] = L1cache[L1_B_index,:]
                    for i in range(M2):
                        A_ik = A[ii+i,kk+k]
                        C_vr[:] = L1cache[L1_C_base+i,:]
                        T_vr[:] = A_ik
                        T_vr[:] = B_vr[:] * T_vr[:]
                        C_vr[:] = C_vr[:] + T_vr[:]
                        L1cache[L1_C_base+i,:] = C_vr[:]
            for i in range(M2):
                C[ii+i,jj:jj+V] = L1cache[L1_C_base+i,:]

    print(C)
