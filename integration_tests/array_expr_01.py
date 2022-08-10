from ltypes import i32, f32, f64
from numpy import empty, reshape, int32, float64

def array_expr_01():
    dim1: i32
    dim2: i32
    dim3: i32
    dim1d: i32
    i: i32
    shape1d: i32[1] = empty(1, dtype=int32)
    shape3d: i32[3] = empty(3, dtype=int32)
    eps: f64
    eps = 1e-12

    dim1 = 10
    dim2 = 10
    dim3 = 5
    dim1d = dim1 * dim2 * dim3

    e: f64[10, 10, 5] = empty((dim1, dim2, dim3), dtype=float64)
    f: f64[10, 10, 5] = empty((dim1, dim2, dim3), dtype=float64)
    g: f64[500] = empty(dim1d, dtype=float64)
    e1d: f64[500] = empty((dim1d), dtype=float64)
    f1d: f64[500] = empty((dim1d), dtype=float64)

    for i in range(dim1d):
        e1d[i] = float(i + 1)
        f1d[i] = float(i + 1)

    shape3d[0] = dim1
    shape3d[1] = dim2
    shape3d[2] = dim3
    shape1d[0] = dim1d
    e = reshape(e1d, shape3d)
    f = reshape(f1d, shape3d)
    g = reshape(e + f, shape1d)

    for i in range(dim1d):
        assert abs(g[i] - 2*(i + 1)) <= eps

array_expr_01()
