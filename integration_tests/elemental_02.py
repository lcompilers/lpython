from ltypes import i32, f64, f32
from numpy import empty, tan, sin, cos, reshape

def elemental_tan64():
    theta: f64[16, 8, 4, 2, 1] = empty((16, 8, 4, 2, 1))
    theta1d: f64[1024] = empty(1024)
    tantheta: f64[16, 8, 4, 2, 1] = empty((16, 8, 4, 2, 1))
    observed: f64[16, 8, 4, 2, 1] = empty((16, 8, 4, 2, 1))
    shapend: i32[5] = empty(5, dtype=int)
    i: i32
    j: i32
    k: i32
    l: i32
    eps: f64
    eps = 1e-12

    for i in range(1024):
        theta1d[i] = float(i + 1)

    for i in range(5):
        shapend[i] = 2**(4 - i)
    theta = reshape(theta1d, shapend)

    observed = sin(theta)/cos(theta)
    tantheta = tan(theta)

    for i in range(16):
        for j in range(8):
            for k in range(4):
                for l in range(2):
                    assert abs(tantheta[i, j, k, l, 0] - observed[i, j, k, l, 0]) <= eps

def elemental_tan32():
    theta: f32[5, 5] = empty((5, 5))
    theta1d: f32[25] = empty(25)
    tantheta: f32[5, 5] = empty((5, 5))
    observed: f32[5, 5] = empty((5, 5))
    shapend: i32[2] = empty(2, dtype=int)
    i: i32
    j: i32
    eps: f32
    eps = 1e-6

    for i in range(25):
        theta1d[i] = float(i + 1)

    shapend[0] = 5
    shapend[1] = 5
    theta = reshape(theta1d, shapend)

    observed = sin(theta)/cos(theta)
    tantheta = tan(theta)

    for i in range(5):
        for j in range(5):
            assert abs(tantheta[i, j] - observed[i, j]) <= eps

elemental_tan64()
elemental_tan32()
