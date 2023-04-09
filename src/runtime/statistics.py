from lpython import i32, f64, i64, f64, overload


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
    return sum/f64(k)


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

    return sum/f64(k)


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
    return sum/f64(k)


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
    return sum/f64(k)


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

@overload
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
        if x[i] <= 0:
            raise Exception("geometric mean requires a non-empty dataset containing positive numbers")
        product *= float(x[i])

    return product**(1/k)

@overload
def geometric_mean(x: list[i64]) -> f64:
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
        if x[i] <= i64(0):
            raise Exception("geometric mean requires a non-empty dataset containing positive numbers")
        product *= float(x[i])

    return product ** (1 / k)

@overload
def geometric_mean(x: list[f64]) -> f64:
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
        if x[i] <= 0.0:
            raise Exception("geometric mean requires a non-empty dataset containing positive numbers")
        product *= x[i]

    return product**(1/k)

@overload
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
        if x[i] < 0:
            raise Exception("Harmonic mean does not support negative values")
        sum += 1 / x[i]

    return f64(k)/sum

@overload
def harmonic_mean(x: list[i64]) -> f64:
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
        if x[i] == i64(0):
            return 0.0
        if x[i] < i64(0):
            raise Exception("Harmonic mean does not support negative values")
        sum += i64(1) / x[i]
    return f64(k)/sum

@overload
def harmonic_mean(x: list[f64]) -> f64:
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
        if x[i] == 0.0:
            return 0.0
        if x[i] < 0.0:
            raise Exception("Harmonic mean does not support negative values")
        sum += 1.0 / x[i]

    return f64(k) / sum


# TODO: Use generics to support other types.
@overload
def mode(x: list[i32]) -> i32:
    k: i32 = len(x)
    c: i32
    count: dict[i32, i32] = {0: 0}

    # insert keys in the dictionary
    for c in range(k):
        count[x[c]] = 0

    # update the frequencies
    for c in range(k):
        count[x[c]] = count[x[c]] + 1

    max_count: i32 = 0
    ans: i32
    for c in range(k):
        if max_count < count[x[c]]:
            max_count = count[x[c]]
            ans = x[c]
    return ans

@overload
def mode(x: list[i64]) -> i64:
    k: i32 = len(x)
    c: i32
    count: dict[i64, i32] = {i64(0): 0}

    # insert keys in the dictionary
    for c in range(k):
        count[x[c]] = 0

    # update the frequencies
    for c in range(k):
        count[x[c]] = count[x[c]] + 1

    max_count: i32 = 0
    ans: i64
    for c in range(k):
        if max_count < count[x[c]]:
            max_count = count[x[c]]
            ans = x[c]
    return ans


@overload
def variance(x: list[f64]) -> f64:
    """
    Returns the variance of a data sequence of numbers
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
        num += (x[i] - xmean)**2.0
    return num / f64(n-1)

@overload
def variance(x: list[i32]) -> f64:
    """
    Returns the variance of a data sequence of numbers
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
        num += (f64(x[i]) - xmean)**2.0
    return num / f64(n-1)


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
def pvariance(x: list[f64]) -> f64:
    """
    Returns the population variance of a data sequence of numbers
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
        num += (x[i] - xmean)**2.0
    return num / f64(n)

@overload
def pvariance(x: list[i32]) -> f64:
    """
    Returns the population variance of a data sequence of numbers
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
        num += (f64(x[i]) - xmean)**2.0
    return num / f64(n)


@overload
def pstdev(x: list[f64]) -> f64:
    """
    Returns the population standard deviation of a data sequence of numbers
    """
    return pvariance(x)**0.5

@overload
def pstdev(x: list[i32]) -> f64:
    """
    Returns the population standard deviation of a data sequence of numbers
    """
    return pvariance(x)**0.5

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
        sxy += (f64(x[i]) - xmean) * (f64(y[i]) - ymean)

    sxx: f64 = 0.0
    j: i32
    for j in range(n):
        sxx += (f64(x[j]) - xmean) ** 2.0

    syy: f64 = 0.0
    k: i32
    for k in range(n):
        syy += (f64(y[k]) - ymean) ** 2.0
    if (sxx * syy) == 0.0:
        raise Exception('at least one of the inputs is constant')
    return sxy / (sxx * syy)**0.5

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
        sxx += (f64(x[j]) - xmean) ** 2.0

    syy: f64 = 0.0
    k: i32
    for k in range(n):
        syy += (f64(y[k]) - ymean) ** 2.0
    if (sxx * syy) == 0.0:
        raise Exception('at least one of the inputs is constant')
    return sxy / (sxx * syy)**0.5

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
        num += (f64(x[i]) - xmean) * (f64(y[i]) - ymean)
    return num / f64(n-1)

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
    return num / f64(n-1)

@overload
def linear_regression(x: list[i32], y: list[i32]) -> tuple[f64, f64]:

    """
    Returns the slope and intercept of simple linear regression
    parameters estimated using ordinary least squares.
    """
    n: i32 = len(x)
    if len(y) !=  n:
        raise Exception('linear regression requires that both inputs have same number of data points')
    if n < 2:
        raise Exception('linear regression requires at least two data points')
    xmean: f64 = mean(x)
    ymean: f64 = mean(y)

    sxy: f64 = 0.0
    i: i32
    for i in range(n):
        sxy += (f64(x[i]) - xmean) * (f64(y[i]) - ymean)

    sxx: f64 = 0.0
    j: i32
    for j in range(n):
        sxx += (f64(x[j]) - xmean) ** 2.0

    slope: f64

    if sxx == 0.0:
        raise Exception('x is constant')
    else:
        slope = sxy / sxx

    intercept: f64  = ymean - slope * xmean

    LinReg: tuple[f64, f64] = (slope, intercept)

    return LinReg

@overload
def linear_regression(x: list[f64], y: list[f64]) -> tuple[f64, f64]:

    """
    Returns the slope and intercept of simple linear regression
    parameters estimated using ordinary least squares.
    """
    n: i32 = len(x)
    if len(y) !=  n:
        raise Exception('linear regression requires that both inputs have same number of data points')
    if n < 2:
        raise Exception('linear regression requires at least two data points')
    xmean: f64 = mean(x)
    ymean: f64 = mean(y)

    sxy: f64 = 0.0
    i: i32
    for i in range(n):
        sxy += (x[i] - xmean) * (y[i] - ymean)

    sxx: f64 = 0.0
    j: i32
    for j in range(n):
        sxx += (f64(x[j]) - xmean) ** 2.0

    slope: f64

    if sxx == 0.0:
        raise Exception('x is constant')
    else:
        slope = sxy / sxx

    intercept: f64  = ymean - slope * xmean

    LinReg: tuple[f64, f64] = (slope, intercept)

    return LinReg
