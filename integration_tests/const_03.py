from ltypes import i64, f64, i32, Const, ccall

CONST_1: Const[f64] = 32.0
CONST_2: Const[f64] = CONST_1 * 2.0
CONST_3: Const[i64] = i64(CONST_1 + CONST_2)

CONST_11: Const[i32] = 32
CONST_12: Const[i32] = CONST_11

def print_value(value: Const[f64]):
    print(value)

@ccall
def print_value_c(value: Const[f64]):
    pass

def test_global_consts():
    print_value(CONST_1)
    print_value(CONST_2)
    print(CONST_12)
    print_value_c(CONST_1)
    print_value_c(CONST_2)
    assert CONST_1 == 32.0
    assert CONST_2 == 64.0
    assert f64(abs(CONST_3 - i64(96))) < 1e-12

test_global_consts()
