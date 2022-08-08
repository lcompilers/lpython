from ltypes import i32, f64, overload


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
        sum += x[i]
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
        sum += x[i]

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
        sum += x[i]
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
        product *= x[i]

    return product**(1/k)


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
    return k/sum
