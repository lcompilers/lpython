from decimal import (Decimal)
from ltypes import i64, f32, f64, i64

eps: f64
eps = 1e-12

def test_Decimal_1():
    i: Decimal
    a: i64
    a = 4
    i = Decimal(a)
    assert i == 4
    b: i64
    b = 2
    j: Decimal
    j = Decimal(b)
    k: Decimal
    c: f64
    c = 0.5
    k = Decimal(c)
    assert abs(float(j ** k) - 1.414213562373095048801688724) < eps
    assert i**j == Decimal(16)

def check():
    test_Decimal_1()
check()
