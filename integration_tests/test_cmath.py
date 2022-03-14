from cmath import exp, log, sqrt
from ltypes import c64

def test_power_logarithmic():
    x: c64
    y: c64
    x = complex(3, 3)
    y = exp(x)
    y = log(x)
    y = sqrt(x)

test_power_logarithmic()
