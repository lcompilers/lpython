from lpython import i32, f64, dataclass, field
from numpy import empty, float64

@dataclass
class Mat:
    mat: f64[2, 2] = field(default_factory=lambda: empty((2, 2), dtype=float64))

@dataclass
class Vec:
    vec: f64[2] = field(default_factory=lambda: empty(2, dtype=float64))

@dataclass
class MatVec:
    mat: Mat = field(default_factory=lambda: Mat())
    vec: Vec = field(default_factory=lambda: Vec())

def rotate(mat_vec: MatVec) -> f64[2]:
    rotated_vec: f64[2] = empty(2, dtype=float64)
    rotated_vec[0] = mat_vec.mat.mat[0, 0] * mat_vec.vec.vec[0] + mat_vec.mat.mat[0, 1] * mat_vec.vec.vec[1]
    rotated_vec[1] = mat_vec.mat.mat[1, 0] * mat_vec.vec.vec[0] + mat_vec.mat.mat[1, 1] * mat_vec.vec.vec[1]
    return rotated_vec

def create_MatVec_obj() -> MatVec:
    mat: f64[2, 2] = empty((2, 2), dtype=float64)
    vec: f64[2] = empty(2, dtype=float64)
    mat[0, 0] = 0.0
    mat[0, 1] = -1.0
    mat[1, 0] = 1.0
    mat[1, 1] = 0.0
    vec[0] = 1.0
    vec[1] = 0.0
    mat_s: Mat = Mat(mat)
    vec_s: Vec = Vec(vec)
    mat_vec: MatVec = MatVec(mat_s, vec_s)
    return mat_vec

def test_rotate_by_90():
    mat_vec: MatVec = create_MatVec_obj()
    print(mat_vec.mat.mat[0, 0], mat_vec.mat.mat[0, 1], mat_vec.mat.mat[1, 0], mat_vec.mat.mat[1, 1])
    print(mat_vec.vec.vec[0], mat_vec.vec.vec[1])
    rotated_vec: f64[2] = rotate(mat_vec)
    print(rotated_vec[0], rotated_vec[1])
    assert abs(rotated_vec[0] - 0.0) <= 1e-12
    assert abs(rotated_vec[1] - 1.0) <= 1e-12

test_rotate_by_90()
