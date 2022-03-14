from cmath import exp, log, sqrt, acos, asin, atan, cos, sin, tan
from ltypes import c64

def test_power_logarithmic():
    x: c64
    y: c64
    x = complex(3, 3)
    y = exp(x)
    y = log(x)
    y = sqrt(x)


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


test_power_logarithmic()
test_trigonometric()
