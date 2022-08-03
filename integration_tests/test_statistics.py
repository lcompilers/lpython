from statistics import (mean, geometric_mean, harmonic_mean, fmean, median, median_low, median_high)
from ltypes import i32, f64, i64

eps: f64
eps = 1e-12

def test_mean():
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

    d: list[i32]
    d = [1, 3, 11]
    l: f64
    l = mean(d)
    assert abs(l - 5) < eps

def test_fmean():
    a: list[i32]
    a = [9, 4, 10]
    j: f64
    j = fmean(a)
    assert abs(j - 7.666666666666667) < eps

    b: list[f64]
    b = [-1.9, -13.8, -6, 4.2, 5.9, 9.1]
    k: f64
    k = fmean(b)
    assert abs(k + 0.41666666666666674) < eps


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

def test_median():
    a: list[i32]
    a = [9, 4, 10]
    j: f64
    j = median(a)
    assert abs(j - 9) < eps

    b: list[i32]
    b = [9, 4, 1, 10]
    k: f64
    k = median(b)
    assert abs(k - 6.5) < eps

    c: list[f64]
    c = [-1.2, 3.4, 9.7, 6.0, 5.1]
    l: f64
    l = median(c)
    assert abs(l - 5.1) < eps

def test_median_low():
    a: list[i32]
    a = [9, 4, 10]
    j: f64
    j = median_low(a)
    assert abs(j - 9) < eps

    b: list[i32]
    b = [9, 4, 1, 10]
    k: f64
    k = median_low(b)
    assert abs(k - 4) < eps

    c: list[f64]
    c = [-1.2, 3.4, 6.0, 5.1]
    l: f64
    l = median_low(c)
    assert abs(l - 3.4) < eps

def test_median_high():
    a: list[i32]
    a = [9, 4, 10]
    j: f64
    j = median_high(a)
    assert abs(j - 9) < eps

    b: list[i32]
    b = [9, 4, 1, 10]
    k: f64
    k = median_high(b)
    assert abs(k - 9) < eps

    c: list[f64]
    c = [-1.2, 3.4, 6.0, 5.1]
    l: f64
    l = median_high(c)
    assert abs(l - 5.1) < eps


def check():
    test_mean()
    test_geometric_mean()
    test_harmonic_mean()
    test_fmean()
    test_median()
    test_median_low()
    test_median_high()

check()
