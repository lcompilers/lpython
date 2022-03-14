from ltypes import f64, i32
from math import sqrt

def _sum(x: list[f64]) -> f64:
    result: f64
    result = 0.0
    e: i32
    for e in range(len(x)):
        result += x[e]
    return result


def mean(data: list[f64]) -> f64:
    """
    Return the sample arithmetic mean of data.
    """
    n: i32
    n = len(data)
    return _sum(data)/n


def fmean(data: list[f64]) -> f64:
    """
    Convert data to floats and compute the arithmetic mean.
    """
    return mean(data)


def variance(data: list[f64]) -> f64:
    """
    Sample variance of data.
    """
    n: i32
    n = len(data)
    if n < 1:
        raise Exception("n > 1 for variance")
    xbar: f64
    xbar = mean(data)
    num: f64
    num = 0.0
    i: i32
    for i in range(n):
        num += (data[i]-xbar)**2
    return num/(n-1)


def stdev(data: list[f64]) -> f64:
    """
    Sample standard deviation of data.
    """
    return sqrt(variance(data))
