from cmath import (exp, log, sqrt, acos, asin, atan, cos, sin, tan,
                   acosh, asinh, atanh, cosh, sinh, tanh)
from lpython import c64, c32

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


test_power_logarithmic()
test_trigonometric()
test_hyperbolic()
