from ltypes import i32
from numpy import empty, int32

def main0():
    x: i32 = 2
    arr1: i32[x] = empty(x, dtype=int32)
    arr2: i32[x] = empty(x, dtype=int32)
    arr1[0] = 100
    arr1[1] = 200
    arr2[0] = 300
    arr2[1] = 400
    print(arr1, arr2)
    print(arr1, arr2, sep="abc")
    print(arr1, arr2, end="pqr\n")
    print(arr1, arr2, sep="abc", end="pqr\n")

main0()