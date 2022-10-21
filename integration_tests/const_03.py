from ltypes import i64, f64, Const

CONST_1: Const[f64] = 32.0
CONST_2: Const[f64] = CONST_1 * 2.0
CONST_3: Const[i64] = CONST_1 + CONST_2

def test_global_consts():
    assert CONST_1 == 32
    assert CONST_2 == 64
    assert abs(CONST_3 - 96.0) < 1e-12

test_global_consts()
