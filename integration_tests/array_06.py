from numpy import empty, int16
from lpython import i16, i32, Const

def spot_print_lpython_array(a: i16[:,:]) -> None:
    print(a)

def main() -> i32:
    n  : Const[i32] = 15
    m  : Const[i32] = 3
    Anm: i16[n, m] = empty((n,m), dtype=int16)
    i: i32; j: i32
    for i in range(n):
        for j in range(m):
            Anm[i,j] = i16(5)
    spot_print_lpython_array(Anm)
    return 0

if __name__ == "__main__":
    main()
