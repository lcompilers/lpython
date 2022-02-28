from math import (sin, cos, tan, pi, sqrt, log, log10, log2, erf, erfc, gamma,
                  lgamma)

def test_trig():
    # TODO: importing from `math` doesn't work here yet:
    pi: f64 = 3.141592653589793238462643383279502884197
    eps: f64 = 1e-12
    assert abs(sin(0.0)-0) < eps
    assert abs(sin(pi/2)-1) < eps
    assert abs(cos(0.0)-1) < eps
    assert abs(cos(pi/2)-0) < eps
    assert abs(tan(0.0)-0) < eps
    assert abs(tan(pi/4)-1) < eps

def test_sqrt():
    eps: f64 = 1e-12
    assert abs(sqrt(2.0) - 1.4142135623730951) < eps
    assert abs(sqrt(9.0) - 3) < eps

def test_log():
    eps: f64 = 1e-12
    assert abs(log(1.0) - 0) < eps
    assert abs(log(2.718281828459) - 1) < eps
    assert abs(log2(2.0) - 1) < eps
    assert abs(log10(10.0) - 1) < eps

def test_special():
    eps: f64 = 1e-12
    assert abs(erf(2.0) - 0.9953222650189527) < eps
    assert abs(erfc(2.0) - 0.0046777349810472645) < eps
    assert abs(erf(2.0) + erfc(2.0) - 1) < eps
    assert abs(gamma(5.0) - 24) < eps
    assert abs(lgamma(5.0) - log(gamma(5.0))) < eps


test_trig()
test_sqrt()
test_log()
test_special()
