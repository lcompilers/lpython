from ltypes import f64, i32

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
