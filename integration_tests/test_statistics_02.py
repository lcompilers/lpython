from statistics import (covariance, correlation,
                        linear_regression)
from lpython import i32, f64


eps: f64
eps = 1e-12

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
    test_linear_regression()
    test_correlation()
    test_covariance()

check()
