from lpython import i32
from numpy import empty, int32

def main0():
    Nx: i32 = 60; Ny: i32 = 45; Nz: i32 = 20
    arr: i32[45, 60, 20] = empty([45, 60, 20], dtype=int32)
    i: i32
    j: i32
    k: i32
    for i in range(Ny):
        for j in range(Nx):
            for k in range(Nz):
                arr[i, j, k] = i * j + k
                if k & 1:
                    arr[i, j, k] = -arr[i, j, k]


    for i in range(Ny):
        for j in range(Nx):
            for k in range(Nz):
                print(i, j, k, int(arr[i, j, k]))

main0()
