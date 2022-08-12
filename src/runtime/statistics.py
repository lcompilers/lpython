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
        if(x[i]<=0):
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
        if(x[i]<=0):
            raise Exception("geometric mean requires a non-empty dataset containing positive numbers")
        product *= float(x[i])

    return product**(1/k)

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
        if(x[i]<=0.0):
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
        if(x[i]<0.0):
            raise Exception("Harmonic mean does not support negative values")
        sum += 1 / x[i]

    return float(k/sum)
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
        if x[i] == 0:
            return 0.0
        if(x[i]<0):
            raise Exception("Harmonic mean does not support negative values")
        sum += 1 / x[i]
    return k/sum

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
        if(x[i]<0.0):
            raise Exception("Harmonic mean does not support negative values")
        sum += 1 / x[i]

    return k/sum

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
        num += (x[i]-xmean)**2
    return num/(n-1)

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
        num += (x[i]-xmean)**2
    return num/(n-1)


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

