from ltypes import Const, i32, i64, f32, f64

def test_const_variables():
    xci: Const[i32] = 0.0
    xi: i32 = 0

    yci: Const[i64] = int(1)
    yi: i64 = int(1)

    xcf: Const[f32] = 2
    xf: f32 = 2.0

    ycf: Const[f64] = 3.0
    yf: f64 = 3.0

    assert xci == xi
    assert yci == yi
    assert xcf == xf
    assert ycf == yf

test_const_variables()
