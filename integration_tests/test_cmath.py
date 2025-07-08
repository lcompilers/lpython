from cmath import (acos, acosh, asin, asinh, atan, atanh, cos, cosh, exp, log,
                   phase, polar, rect, sin, sinh, sqrt, tan, tanh)

from lpython import c32, c64, f64


def test_power_logarithmic():
    x: c64
    y: c64
    x = complex(3, 3)
    y = exp(x)
    y = log(x)
    y = sqrt(x)
    a: c32
    b: c32
    a = c32(complex(3, 3))
    b = exp(a)
    b = log(a)
    b = sqrt(a)


def test_trigonometric():
    x: c64
    y: c64
    x = complex(3, 3)
    y = acos(x)
    y = asin(x)
    y = atan(x)
    y = cos(x)
    y = sin(x)
    y = tan(x)
    a: c32
    b: c32
    a = c32(complex(3, 3))
    b = acos(a)
    b = asin(a)
    b = atan(a)
    b = cos(a)
    b = sin(a)
    b = tan(a)


def test_hyperbolic():
    x: c64
    y: c64
    x = complex(3, 3)
    y = acosh(x)
    y = asinh(x)
    y = atanh(x)
    y = cosh(x)
    y = sinh(x)
    y = tanh(x)
    a: c32
    b: c32
    a = c32(complex(3, 3))
    b = acosh(a)
    b = asinh(a)
    b = atanh(a)
    b = cosh(a)
    b = sinh(a)
    b = tanh(a)


def test_polar():
    x: c64
    eps: f64
    eps = 1e-6
    x = complex(1, -2)
    assert f64(abs(f64(phase(x)) - (-1.1071487177940904))) < eps
    assert f64(abs(f64(polar(x)[0]) - (2.23606797749979))) < eps
    assert abs(abs(rect(2.23606797749979, -1.1071487177940904))-abs(x)) < eps


test_power_logarithmic()
test_trigonometric()
test_hyperbolic()
test_polar()
