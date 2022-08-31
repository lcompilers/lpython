from statistics import (mean, fmean, geometric_mean, harmonic_mean, variance,
                        stdev, pvariance, pstdev, correlation, covariance, linear_regression)
from ltypes import i32, f64, i64, f32

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


def test_covariance():
    a: list[i32]
    a = [1, 2, 3, 4, 5, 6, 7, 8, 9]
    b: list[i32]
    b = [1, 2, 3, 1, 2, 3, 1, 2, 3]
    j: f64
    j = covariance(a, b)
    assert abs(j - 0.75) < eps

    c: list[f64]
    c = [2.74, 1.23, 2.63, 2.22, 3.0, 1.98]
    d: list[f64]
    d = [9.4, 1.23, 2.63, 22.4, 1.9, 13.98]
    k: f64
    k = covariance(c, d)
    assert abs(k + 0.24955999999999934) < eps


def test_correlation():
    a: list[i32]
    a = [11, 2, 7, 4, 15, 6, 10, 8, 9, 1, 11, 5, 13, 6, 15]
    b: list[i32]
    b = [2, 5, 17, 6, 10, 8, 13, 4, 6, 9, 11, 2, 5, 4, 7]

    j: f64
    j = correlation(a, b)
    assert abs(j - 0.11521487988958108) < eps

    c: list[f64]
    c = [2.0, 23.0, 24.55, 64.436, 5403.23]
    d: list[f64]
    d = [26.9, 75.6, 34.06, 356.89, 759.26]

    j = correlation(c, c)
    assert abs(j - 1.0) < eps

    j = correlation(c, d)
    assert abs(j - 0.9057925526720572) < eps

def test_linear_regression():
    c: list[f64]
    c = [2.74, 1.23, 2.63, 2.22, 3.0, 1.98]
    d: list[f64]
    d = [9.4, 1.23, 2.63, 22.4, 1.9, 13.98]

    slope: f64
    intercept: f64
    slope, intercept = linear_regression(c, d)

    assert abs(slope + 0.6098133124816717) < eps
    assert abs(intercept - 9.992570618707845) < eps

    a: list[i32]
    b: list[i32]
    a = [12, 24, 2, 1, 43, 53, 23]
    b = [2, 13, 14, 63, 49, 7, 3]

    slope, intercept = linear_regression(a, b)

    assert abs(slope + 0.18514007308160782) < eps
    assert abs(intercept - 25.750304506699152) < eps

def check():
    test_mean()
    test_geometric_mean()
    test_harmonic_mean()
    test_fmean()
    test_variance()
    test_stdev()
    test_pvariance()
    test_pstdev()
    test_linear_regression()
    test_correlation()
    test_covariance()

check()
