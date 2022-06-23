from decimal import (Decimal)
from ltypes import i32, f32, f64, i64

eps: f64
eps = 1e-12

def test_Decimal_1():
    i: i32
    i = Decimal(4)
    assert i == 4
    j: i64
    j = Decimal(2)
    k: f64
    k = Decimal(0.5)
    assert abs(float(j ** k) - 1.414213562373095048801688724) < eps

def check():
    test_Decimal_1()
check()