from ltypes import i32, f64, overload
from math import sqrt


@overload
def mean(x: list[i32]) -> f64:
    """
    Returns the arithmetic mean of a data sequence of numbers
    """
    k: i32 = len(x)
    if k == 0:
        return 0.0
    sum: f64
    sum = 0.0
    i: i32

    for i in range(k):
        sum += float(x[i])
    return sum/k


@overload
def mean(x: list[i64]) -> f64:
    """
    Returns the arithmetic mean of a data sequence of numbers
    """
    k: i32 = len(x)
    if k == 0:
        return 0.0
    sum: f64
    sum = 0.0
    i: i32

    for i in range(k):
        sum += float(x[i])

    return sum/k


@overload
def mean(x: list[f32]) -> f64:
    """
    Returns the arithmetic mean of a data sequence of numbers
    """
    k: i32 = len(x)
    if k == 0:
        return 0.0
    sum: f64
    sum = 0.0
    i: i32

    for i in range(k):
        sum += float(x[i])
    return sum/k


@overload
def mean(x: list[f64]) -> f64:
    """
    Returns the arithmetic mean of a data sequence of numbers
    """
    k: i32 = len(x)
    if k == 0:
        return 0.0
    sum: f64
    sum = 0.0
    i: i32

    for i in range(k):
        sum += x[i]
    return sum/k


@overload
def fmean(x: list[i32]) -> f64:
    """
    Returns the floating type arithmetic mean of a data sequence of numbers
    """
    return mean(x)


@overload
def fmean(x: list[i64]) -> f64:
    """
    Returns the floating type arithmetic mean of a data sequence of numbers
    """
    return mean(x)


@overload
def fmean(x: list[f64]) -> f64:
    """
    Returns the floating type arithmetic mean of a data sequence of numbers
    """
    return mean(x)


@overload
def fmean(x: list[f32]) -> f64:
    """
    Returns the floating type arithmetic mean of a data sequence of numbers
    """
    return mean(x)


def geometric_mean(x: list[i32]) -> f64:
    """
    Returns the geometric mean of a data sequence of numbers
    """
    k: i32 = len(x)
    if k == 0:
        return 0.0
    product: f64
    product = 1.0
    i: i32

    for i in range(k):
        product *= float(x[i])

    return product ** (1 / k)


def harmonic_mean(x: list[i32]) -> f64:
    """
    Returns the harmonic mean of a data sequence of numbers
    """
    k: i32 = len(x)
    if k == 0:
        return 0.0
    sum: f64
    sum = 0.0
    i: i32

    for i in range(k):
        if x[i] == 0:
            return 0.0
        sum += 1 / x[i]

    return k / sum

@overload
def variance(x: list[f64]) -> f64:
    """
    Returns the varience of a data sequence of numbers
    """
    n: i32
    n = len(x)
    if n < 1:
        raise Exception("n > 1 for variance")
    xmean: f64
    xmean = mean(x)
    num: f64
    num = 0.0
    i: i32
    for i in range(n):
        num += (x[i] - xmean)**2
    return num / (n-1)

@overload
def variance(x: list[i32]) -> f64:
    """
    Returns the varience of a data sequence of numbers
    """
    n: i32
    n = len(x)
    if n < 1:
        raise Exception("n > 1 for variance")
    xmean: f64
    xmean = mean(x)
    num: f64
    num = 0.0
    i: i32
    for i in range(n):
        num += (x[i] - xmean)**2
    return num / (n-1)


@overload
def stdev(x: list[f64]) -> f64:
    """
    Returns the standard deviation of a data sequence of numbers
    """
    return variance(x)**0.5

@overload
def stdev(x: list[i32]) -> f64:
    """
    Returns the standard deviation of a data sequence of numbers
    """
    return variance(x)**0.5

@overload
def covariance(x: list[i32], y: list[i32]) -> f64:
    """
    Returns the covariance of a data sequence of numbers
    """
    n: i32 = len(x)
    m: i32 = len(y)
    if (n < 2 or m < 2) or n != m:
        raise Exception("Both inputs must be of the same length (no less than two)")
    xmean: f64 = mean(x)
    ymean: f64 = mean(y)
    num: f64
    num = 0.0
    i: i32
    for i in range(n):
        num += (x[i] - xmean) * (y[i] - ymean)
    return num / (n-1)

@overload
def covariance(x: list[f64], y: list[f64]) -> f64:
    """
    Returns the covariance of a data sequence of numbers
    """
    n: i32 = len(x)
    m: i32 = len(y)
    if (n < 2 or m < 2) or n != m:
        raise Exception("Both inputs must be of the same length (no less than two)")
    xmean: f64 = mean(x)
    ymean: f64 = mean(y)
    num: f64
    num = 0.0
    i: i32
    for i in range(n):
        num += (x[i] - xmean) * (y[i] - ymean)
    return num / (n-1)

@overload
def correlation(x: list[i32], y: list[i32]) -> f64:
    """
    Return the Pearson's correlation coefficient for two inputs.
    """
    n: i32 = len(x)
    m: i32 = len(y)
    if n != m:
        raise Exception("correlation requires that both inputs have same number of data points")
    if n < 2:
        raise Exception("correlation requires at least two data points")
    xmean: f64 = mean(x)
    ymean: f64 = mean(y)

    sxy: f64 = 0.0
    i: i32
    for i in range(n):
        sxy += (x[i] - xmean) * (y[i] - ymean)

    sxx: f64 = 0.0
    j: i32
    for j in range(n):
        sxx += (x[j] - xmean) ** 2

    syy: f64 = 0.0
    k: i32
    for k in range(n):
        syy += (y[k] - ymean) ** 2
    if sqrt(sxx * syy) == 0:
        raise Exception('at least one of the inputs is constant')
    return sxy / sqrt(sxx * syy)

@overload
def correlation(x: list[f64], y: list[f64]) -> f64:
    """
    Return the Pearson's correlation coefficient for two inputs.
    """
    n: i32 = len(x)
    m: i32 = len(y)
    if n != m:
        raise Exception("correlation requires that both inputs have same number of data points")
    if n < 2:
        raise Exception("correlation requires at least two data points")
    xmean: f64 = mean(x)
    ymean: f64 = mean(y)

    sxy: f64 = 0.0
    i: i32
    for i in range(n):
        sxy += (x[i] - xmean) * (y[i] - ymean)

    sxx: f64 = 0.0
    j: i32
    for j in range(n):
        sxx += (x[j] - xmean) ** 2

    syy: f64 = 0.0
    k: i32
    for k in range(n):
        syy += (y[k] - ymean) ** 2
    if sqrt(sxx * syy) == 0:
        raise Exception('at least one of the inputs is constant')
    return sxy / sqrt(sxx * syy)

