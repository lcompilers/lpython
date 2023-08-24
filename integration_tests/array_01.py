from lpython import i32
from numpy import empty, int32

def main0():
    Nx: i32 = 600; Ny: i32 = 450
    arr: i32[450, 600] = empty([450, 600], dtype=int32)
    i: i32
    j: i32
    for i in range(Ny):
        for j in range(Nx):
            arr[i, j] = i * j

    for i in range(Ny):
        for j in range(Nx):
            print(i, j, int(arr[i, j]))

main0()
