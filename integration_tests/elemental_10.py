from lpython import i32, i64, f64
from numpy import mod, int64, empty

def test_numpy_mod():
    q1: i64[32, 16, 7] = empty((32, 16, 7), dtype=int64)
    d1: i64[32, 16, 7] = empty((32, 16, 7), dtype=int64)
    r1: i64[32, 16, 7] = empty((32, 16, 7), dtype=int64)
    r1neg: i64[32, 16, 7] = empty((32, 16, 7), dtype=int64)
    q2: i64[100] = empty(100, dtype=int64)
    d2: i64[100] = empty(100, dtype=int64)
    r2: i64[100] = empty(100, dtype=int64)
    i: i32; j: i32; k: i32
    rem: i64; q: i64; d: i64

    for i in range(32):
        for j in range(16):
            for k in range(7):
                d1[i, j, k] = i64(k + 1)
                q1[i, j, k] = i64((i + j) * (k + 1) + k)

    r1 = mod(q1, d1)
    r1neg = mod(-q1, d1)

    for i in range(32):
        for j in range(16):
            for k in range(7):
                assert r1[i, j, k] == i64(k)
                if k == 0:
                    rem = i64(0)
                else:
                    rem = d1[i, j, k] - i64(k)
                assert r1neg[i, j, k] == rem

    for i in range(32):
        for j in range(16):
            for k in range(7):
                d1[i, j, k] = i64(k + 2)
                q1[i, j, k] = i64(i + j)

    r1 = mod(d1 * q1 + r1 + i64(1), d1)

    for i in range(32):
        for j in range(16):
            for k in range(7):
                assert r1[i, j, k] == i64(k + 1)

    r1 = mod(i64(2) * q1 + i64(1), int(2))

    for i in range(32):
        for j in range(16):
            for k in range(7):
                assert r1[i, j, k] == i64(1)

    for i in range(100):
        d2[i] = i64(i + 1)

    r2 = mod(int(100), d2)

    for i in range(100):
        assert r2[i] == i64(100 % (i + 1))

    for i in range(100):
        d2[i] = i64(50 - i)
        q2[i] = i64(39 - i)

    r2 = mod(q2, d2)

    for i in range(100):
        d = i64(50 - i)
        q = i64(39 - i)
        rem = r2[i]
        if d == i64(0):
            assert rem == i64(0)
        else:
            assert f64(int((q - rem)/d)) - (q - rem)/d == 0.0


test_numpy_mod()
