from lpython import i32
from numpy import empty

def main0():
    Nx: i32 = 600; Ny: i32 = 450
    arr: i32[450, 600] = empty([Ny, Nx])
    i: i32
    j: i32
    for i in range(Ny):
        for j in range(Nx):
            arr[i, j] = i * j

    for i in range(Ny):
        for j in range(Nx):
            print(i, j, int(arr[i, j]))

main0()
