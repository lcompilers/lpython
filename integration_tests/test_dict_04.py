from ltypes import i32, i64, f64
from math import pi, sin, cos

def test_dict():
    terms2poly: dict[tuple[i32, i32], i64] = {}
    rtheta2coords: dict[tuple[i64, i64], tuple[f64, f64]] = {}
    i: i32
    n: i64
    size: i32 = 7000
    size1: i32
    theta: f64
    r: f64
    coords: tuple[f64, f64]
    eps: f64 = 1e-12

    n = i64(0)
    for i in range(1000, 1000 + size, 7):
        terms2poly[(i, i*i)] = int(i + i*i)

        theta = float(n) * pi
        r = float(i)
        rtheta2coords[(int(i), n)] = (r * sin(theta), r * cos(theta))

        n += int(1)

    size1 = i32(size/7)
    n = i64(0)
    for i in range(1000, 1000 + size//2, 7):
        assert terms2poly.pop((i, i*i)) == int(i + i*i)

        theta = float(n) * pi
        r = float(i)
        coords = rtheta2coords.pop((int(i), n))
        assert abs(coords[0] - r * sin(theta)) <= eps
        assert abs(coords[1] - r * cos(theta)) <= eps

        size1 = size1 - 1
        assert len(terms2poly) == size1
        n += int(1)

    n = i64(0)
    for i in range(1000, 1000 + size//2, 7):
        terms2poly[(i, i*i)] = int(1 + 2*i + i*i)

        theta = float(n) * pi
        r = float(i)
        rtheta2coords[(int(i), n)] = (r * cos(theta), r * sin(theta))

        n += int(1)

    n = i64(0)
    for i in range(1000, 1000 + size//2, 7):
        assert terms2poly[(i, i*i)] == i64((i + 1)*(i + 1))

        theta = float(n) * pi
        r = float(i)
        assert abs(rtheta2coords[(int(i), n)][0] - r * cos(theta)) <= eps
        assert abs(rtheta2coords[(int(i), n)][1] - r * sin(theta)) <= eps

        n += int(1)

    n = i64(0)
    for i in range(1000, 1000 + size, 7):
        terms2poly[(i, i*i)] = int(1 + 2*i + i*i)

        theta = float(n) * pi
        r = float(i)
        rtheta2coords[(int(i), n)] = (r * cos(theta), r * sin(theta))
        n += int(1)

    n = i64(0)
    for i in range(1000, 1000 + size, 7):
        assert terms2poly[(i, i*i)] == i64((i + 1)*(i + 1))

        theta = float(n) * pi
        r = float(i)
        assert abs(r**2.0 - rtheta2coords[(int(i), n)][0]**2.0 - r**2.0 * sin(theta)**2.0) <= eps
        n += int(1)

test_dict()
