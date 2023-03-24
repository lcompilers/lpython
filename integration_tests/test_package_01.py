from nrp import newton_raphson
from ltypes import f64, i32


def check():
    x0: f64 = 20.0
    c: f64 = 3.0
    maxiter: i32 = 20
    x: f64
    x = newton_raphson(x0, c, maxiter)
    assert abs(x - 3.0) < 1e-5

check()
