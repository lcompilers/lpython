from statistics import (mean, fmean, geometric_mean, harmonic_mean,
                        variance, stdev, covariance, correlation)
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
    b = [-1.9, -13.8, -6.0, 4.2, 5.9, 9.1]
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


def test_variance():
    a: list[i32]
    a = [1, 2, 5, 4, 8, 9, 12]
    j: f64
    j = variance(a)
    assert abs(j - 15.80952380952381) < eps

    b: list[f64]
    b = [2.74, 1.23, 2.63, 2.22, 3.0, 1.98]
    k: f64
    k = variance(b)
    assert abs(k - 0.40924) < eps

def test_covariance():
    a: list[i32]
    a = [1, 2, 3, 4, 5, 6, 7, 8, 9]
    b: list[i32]
    b = [1, 2, 3, 1, 2, 3, 1, 2, 3]
    j: f64
    j = covariance(a,b)
    assert abs(j - 0.75) < eps

    c: list[f64]
    c = [2.74, 1.23, 2.63, 2.22, 3.0, 1.98]
    d: list[f64]
    d = [9.4, 1.23, 2.63, 22.4, 1.9, 13.98]
    k: f64
    k = covariance(c,d)
    assert abs(k + 0.24955999999999934) < eps

def test_correlation():
    a: list[i32]
    a = [11, 2, 7, 4, 15, 6, 10, 8, 9, 1, 11, 5, 13, 6, 15]
    b: list[i32]
    b = [2, 5, 17, 6, 10, 8, 13, 4, 6, 9, 11, 2, 5, 4, 7]

    j: f64
    j = correlation(a,b)
    assert abs(j - 0.11521487988958108) < eps

    c: list[i32]
    c = [1, 2, 3, 4, 5, 6, 7, 8, 9]
    d: list[i32]
    d = [9, 8, 7, 6, 5, 4, 3, 2, 1]

    k: f64
    k = correlation(c,c)
    assert k == 1.0

    l: f64
    l = correlation(c,d)
    assert l == -1.0


def test_stdev():
    a: list[i32]
    a = [1, 2, 3, 4, 5]
    j: f64
    j = stdev(a)
    assert abs(j - 1.5811388300841898) < eps

    b: list[f64]
    b = [1.23, 1.45, 2.1, 2.2, 1.9]
    k: f64
    k = stdev(b)
    assert abs(k - 0.41967844833872525) < eps


def check():
    test_mean()
    test_geometric_mean()
    test_harmonic_mean()
    test_fmean()
    test_variance()
    test_stdev()
    test_covariance()
    test_correlation()

check()
