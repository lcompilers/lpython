from ltypes import i32, f64, dataclass
from numpy import empty, float64

@dataclass
class MatVec:
    mat: f64[2, 2]
    vec: f64[2]

def rotate(mat_vec: MatVec) -> f64[2]:
    rotated_vec: f64[2] = empty(2, dtype=float64)
    rotated_vec[0] = mat_vec.mat[0, 0] * mat_vec.vec[0] + mat_vec.mat[0, 1] * mat_vec.vec[1]
    rotated_vec[1] = mat_vec.mat[1, 0] * mat_vec.vec[0] + mat_vec.mat[1, 1] * mat_vec.vec[1]
    return rotated_vec

def test_rotate_by_90():
    mat: f64[2, 2] = empty((2, 2), dtype=float64)
    vec: f64[2] = empty(2, dtype=float64)
    mat[0, 0] = 0.0
    mat[0, 1] = -1.0
    mat[1, 0] = 1.0
    mat[1, 1] = 0.0
    vec[0] = 1.0
    vec[1] = 0.0
    mat_vec: MatVec = MatVec(mat, vec)
    print(mat_vec.mat[0, 0], mat_vec.mat[0, 1], mat_vec.mat[1, 0], mat_vec.mat[1, 1])
    print(mat_vec.vec[0], mat_vec.vec[1])
    rotated_vec: f64[2] = rotate(mat_vec)
    print(rotated_vec[0], rotated_vec[1])
    assert abs(rotated_vec[0] - 0.0) <= 1e-12
    assert abs(rotated_vec[1] - 1.0) <= 1e-12

test_rotate_by_90()
