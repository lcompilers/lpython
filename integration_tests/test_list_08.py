from lpython import i32, f64
from copy import deepcopy

def sort(l: list[i32]) -> list[i32]:
    i: i32; j: i32

    for i in range(len(l)):
        for j in range(i + 1, len(l)):
            if l[i] > l[j]:
                l[i], l[j] = l[j], l[i]

    return l

def sort_list():
    x: list[i32] = []
    size: i32 = 50
    i: i32; j: i32

    for i in range(size):
        x.append(size - i)

    x = deepcopy(sort(x))

    for i in range(size - 1):
        assert x[i] <= x[i + 1]

    assert len(x) == size

def l1norm(v: list[i32]) -> f64:
    result: f64 = 0.0
    i: i32

    for i in range(len(v)):
        result = result + f64(v[i])

    return result

def sort_by_l1_norm():
    mat: list[list[i32]] = []
    vec: list[i32] = []
    i: i32; j: i32; rows: i32; cols: i32; k: i32;
    norm1: f64; norm2: f64;
    rows = 10
    cols = 7
    k = rows * cols

    for i in range(rows):
        for j in range(cols):
            vec.append(k)
            k = k - 1
        mat.append(deepcopy(vec))
        vec.clear()

    for i in range(rows):
        for j in range(i + 1, rows):
            if l1norm(mat[i]) > l1norm(mat[j]):
                mat[i], mat[j] = deepcopy(sort(mat[j])), deepcopy(sort(mat[i]))

    k = 1
    for i in range(rows):
        for j in range(cols):
            assert mat[i][j] == k
            k = k + 1

sort_list()
sort_by_l1_norm()
