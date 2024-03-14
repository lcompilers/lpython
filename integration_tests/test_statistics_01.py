from statistics import (mean, fmean, geometric_mean, harmonic_mean, variance,
                        stdev, pvariance, pstdev, mode, median, median_low)
from lpython import i32, f64, i64


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
    assert abs(l - 5.0) < eps

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

    d: list[f64]
    d = [1.1, 3.4, 17.982, 11.8]
    l: f64
    l = geometric_mean(d)
    assert abs(l - 5.307596520524432) < eps

def test_harmonic_mean():
    c: list[i32]
    c = [9,2,46]
    k: f64
    k = harmonic_mean(c)
    assert abs(k - 4.740458015267175) < eps

    d: list[i32]
    d = [9, 0, 46]
    l: f64
    l = harmonic_mean(d)
    assert l == 0.0

    e: list[f64]
    e = [1.1, 3.4, 17.982, 11.8]
    f: f64
    f = harmonic_mean(e)
    assert abs(f - 2.977152988015106) < eps


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

def test_pvariance():
    a: list[i32]
    a = [1, 2, 5, 4, 8, 9, 12]
    j: f64
    j = pvariance(a)
    assert abs(j - 13.551020408163266) < eps

    b: list[f64]
    b = [2.74, 1.23, 2.63, 2.22, 3.0, 1.98]
    k: f64
    k = pvariance(b)
    assert abs(k - 0.34103333333333335) < eps

def test_pstdev():
    a: list[i32]
    a = [1, 2, 3, 4, 5]
    j: f64
    j = pstdev(a)
    assert abs(j - 1.4142135623730951) < eps

    b: list[f64]
    b = [1.23, 1.45, 2.1, 2.2, 1.9]
    k: f64
    k = pstdev(b)
    assert abs(k - 0.37537181567080935) < eps

def test_mode():
    a: list[i32]
    a = [3, 1, 12, 4, 0]
    i: i32
    i = mode(a)
    assert i == 3

    b: list[i32]
    b = [4, 2, 4, 4, 2, 3, 5]
    j: i32
    j = mode(b)
    assert j == 4

    c: list[i32]
    c = [2, 3, 4, 1, 2, 4, 5]
    k: i32
    k = mode(c)
    assert k == 2

    d: list[i32]
    d = [-1, 2, -3, -5, -3, -1, 4, -2, 4, -5, -3, 4, -3]
    k = mode(d)
    assert k == -3

    e: list[i64]
    e = [i64(-1), i64(2), i64(-3), i64(-5), i64(-3), i64(-1), i64(4), i64(-2), i64(4), i64(-5), i64(-3), i64(4), i64(-3)]
    l: i64 = mode(e)
    assert l == i64(-3)

def test_median():
    a: list[int]
    a = [1, 3, 5, 7, 9]
    j: float
    j = median(a)
    assert j == 5.0

    b: list[int]
    b = [1, 3, 5, 7]
    k: float
    k = median(b)
    assert k == 4.0

    c: list[float]
    c = [1.5, 2.5, 3.5]
    l: float
    l = median(c)
    assert l == 2.5

def test_median_low():
    a: list[int]
    a = [1, 3, 5, 7, 9]
    j: float
    j = median_low(a)
    assert j == 5

    b: list[int]
    b = [1, 3, 5, 7]
    k: float
    k = median_low(b)
    assert k == 3

    c: list[float]
    c = [1.5, 2.5, 3.5]
    l: float
    l = median_low(c)
    assert l == 2.5

def check():
    test_mean()
    test_geometric_mean()
    test_harmonic_mean()
    test_fmean()
    test_variance()
    test_stdev()
    test_pvariance()
    test_pstdev()
    test_mode()
    test_median()
    test_median_low()

check()
