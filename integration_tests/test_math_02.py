from math import (sin, cos, tan, pi, sqrt, cbrt, log, log10, log2, erf, erfc, gamma,
                  lgamma, asin, acos, atan, atan2, asinh, acosh, atanh,
                  tanh, sinh, cosh, hypot, copysign)

from ltypes import i32, f64, i64

pi: f64 = 3.141592653589793238462643383279502884197
e: f64 = 2.718281828459045235360287471352662497757
tau: f64 = 6.283185307179586

eps: f64
eps = 1e-12

def test_trig():
    eps: f64 = 1e-12
    assert abs(sin(0.0)-0) < eps
    assert abs(sin(pi/2)-1) < eps
    assert abs(cos(0.0)-1) < eps
    assert abs(cos(pi/2)-0) < eps
    assert abs(tan(0.0)-0) < eps
    assert abs(tan(pi/4)-1) < eps
    assert abs(asin(1.0) - pi/2) < eps
    assert abs(acos(1.0) - 0) < eps
    assert abs(atan(1.0) - pi/4) < eps
    assert abs(atan2(1.0, 1.0) - pi/4) < eps

def test_acos():
    assert acos(-1) == pi , f"assertion error in acos"
    assert acos(0) == pi/2, f"assertion error in acos"
    assert acos(1) == 0, f"assertion error in acos"

def test_acosh():
    assert acosh(1) == 0 , f"assertion error in acosh"
    assert acosh(2) == 1.3169578969248168, f"assertion error in acosh"
    assert acosh(5.90) == 2.46, f"assertion error in acosh"


def test_asin():
    assert asin(-1) == -pi/2 , f"assertion error in asin"
    assert asin(0) == 0, f"assertion error in asin"
    assert asin(1) == pi/2, f"assertion error in asin"

def test_asinh():
    assert asinh(0) == 0 , f"assertion error in asinh"
    assert asinh(1) == 0.88137358701954305, f"assertion error in asinh"
    assert asinh(-1) == -0.88137358701954305, f"assertion error in asinh"

def test_atan():
    assert atan(-1) == -pi/4 , f"assertion error in atan"
    assert atan(0) == 0, f"assertion error in atan"
    assert atan(1) == pi/4, f"assertion error in atan"

def test_atanh():
    assert atanh(0) == 0 , f"assertion error in atanh"
    assert atanh(0.5) == 0.54930614433405489, f"assertion error in atanh"
    assert atanh(-0.5) == -0.54930614433405489, f"assertion error in atanh"

def test_atan2():
    assert atan2(-1,0) == -pi/2 , f"assertion error in atan2"
    assert atan2(-1,1) == pi/4, f"assertion error in atan2"
    assert atan2(0,1) == 0, f"assertion error in atan2"
    assert atan2(1,1) == pi/4 , f"assertion error in atan2"
    assert atan2(1,0) == pi/2, f"assertion error in atan2"

    assert atan2(0.,-2.3) == pi, f"assertion error in atan2"
    assert atan2(0.,-0.) == pi , f"assertion error in atan2"

    assert atan2(-0.,-2.3) == -pi, f"assertion error in atan2"
    assert atan2(-0.,-0.) == -pi , f"assertion error in atan2"

    assert atan2(2.3,-0) == pi/2, f"assertion error in atan2"
    assert atan2(2.3,0) == pi/2, f"assertion error in atan2"

    assert atan2(-2.3,-0) == -pi/2, f"assertion error in atan2"
    assert atan2(-2.3,0) == -pi/2, f"assertion error in atan2"

def test_cos():
    assert cos(-pi/2) == 0 , f"assertion error in cos"
    assert cos(0) == 1, f"assertion error in cos"
    assert cos(pi/2) == 0, f"assertion error in cos"
    assert cos(pi) == -1, f"assertion error in cos"

def test_cosh():
    assert cosh(0) == 1 , f"assertion error in cosh"
    assert cosh(2)-2*cosh(1)**2 == -1, f"assertion error in cosh"


def test_cbrt():
    assert cbrt(0) == 0 , f"assertion error in cube root function"
    assert cbrt(1) == 1 , f"assertion error in cube root function"
    assert cbrt(8) == 2 , f"assertion error in cube root function"
    assert cbrt(-27) == -3 , f"assertion error in cube root function"
    assert cbrt(1.2) == 1.062658569182611 , f"assertion error in cube root function"


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

def test_hyperbolic():
    eps: f64 = 1e-12
    assert abs(sinh(1.0) - 1.1752011936438014) < eps
    assert abs(cosh(1.0) - 1.5430806348152437) < eps
    assert abs(tanh(1.0) - 0.7615941559557649) < eps
    assert abs(asinh(1.0) - 0.881373587019543) < eps

def test_copysign():
    eps: f64 = 1e-12
    assert abs(copysign(-8.56, 97.21) - 8.56) < eps
    assert abs(copysign(-43.0, -76.67) - (-43.0)) < eps

def test_hypot():
    eps: f64 = 1e-12
    assert abs(hypot(3, 4) - 5.0) < eps
    assert abs(hypot(-3, 4) - 5.0) < eps
    assert abs(hypot(6, 6) - 8.48528137423857) < eps

def check():
    test_acos()
    test_acosh()
    test_asin()
    test_asinh()
    test_atan()
    test_atan2()
    test_atanh()
    test_cos()
    test_cosh()

    test_cbrt()
    test_trig()
    test_sqrt()
    test_log()
    test_special()
    test_hyperbolic()
    test_copysign()
    test_hypot()

check()
