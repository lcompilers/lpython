from statistics import (mean, geometric_mean, harmonic_mean)
from ltypes import i32, f64, i64

eps: f64
eps = 1e-12

def test_mean():
    a: list[i32]
    a = [7, 4, 19]
    i: i32
    i = mean(a)
    assert abs(i - 10) < eps

    b: list[i32]
    b = [9, 4, 10]
    j: f64
    j = mean(b)
    assert abs(j - 7.666666666666667) < eps

    c: list[f64]
    c = [2.0, 3.1, 11.1]
    k: f64
    k = mean(c)
    assert abs(k - 5.4) < eps

def test_geometric_mean():
    c: list[i32]
    c = [1,2,3]
    k: f64
    k = geometric_mean(c)
    assert abs(k - 1.8171205928321397) < eps

def test_harmonic_mean():
    c: list[i32]
    c = [9,2,46]
    k: f64
    k = harmonic_mean(c)
    assert abs(k - 4.740458015267175) < eps


def check():
    test_mean()
    test_geometric_mean()
    test_harmonic_mean()

check()
