import math
from math import pi, e, log
from lpython import i32


def f():
    i: i32
    i = math.factorial(3)
    assert i == 6
    assert abs(pi - 3.14159265358979323846) < 1e-10
    assert abs(e - 2.7182818284590452353) < 1e-10
    assert abs(log(2.0) - 0.69314718055994528) < 1e-10

f()
