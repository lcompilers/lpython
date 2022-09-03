from ltypes import i32, i64
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
                d1[i, j, k] = k + 1
                q1[i, j, k] = (i + j) * (k + 1) + k

    r1 = mod(q1, d1)
    r1neg = mod(-q1, d1)

    for i in range(32):
        for j in range(16):
            for k in range(7):
                assert r1[i, j, k] == k
                if k == 0:
                    rem = 0
                else:
                    rem = d1[i, j, k] - k
                assert r1neg[i, j, k] == rem

    for i in range(32):
        for j in range(16):
            for k in range(7):
                d1[i, j, k] = k + 2
                q1[i, j, k] = i + j

    r1 = mod(d1 * q1 + r1 + 1, d1)

    for i in range(32):
        for j in range(16):
            for k in range(7):
                assert r1[i, j, k] == k + 1

    r1 = mod(2 * q1 + 1, int(2))

    for i in range(32):
        for j in range(16):
            for k in range(7):
                assert r1[i, j, k] == 1

    for i in range(100):
        d2[i] = i + 1

    r2 = mod(int(100), d2)

    for i in range(100):
        assert r2[i] == 100 % (i + 1)

    for i in range(100):
        d2[i] = 50 - i
        q2[i] = 39 - i

    r2 = mod(q2, d2)

    for i in range(100):
        d = 50 - i
        q = 39 - i
        rem = r2[i]
        if d == 0:
            assert rem == 0
        else:
            assert int((q - rem)/d) - (q - rem)/d == 0


test_numpy_mod()
