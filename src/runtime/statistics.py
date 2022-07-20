from ltypes import i32, f64

@overload
def mean(x: list[i32]) -> f64:
    k: i32 = len(x)
    if k == 0:
        return 0.0
    sum: f64
    sum = 0.0
    i: i32

    for i in range(k):
        sum += x[i]
    ans: f64
    ans = sum/k
    return ans

@overload
def mean(x: list[f64]) -> f64:
    k: i32 = len(x)
    if k == 0:
        return 0.0
    sum: f64
    sum = 0.0
    i: i32
    for i in range(k):
        sum += x[i]
    ans: f64
    ans = sum/k
    return ans

@overload
def geometric_mean(x: list[i32]) -> f64:
    k: i32 = len(x)
    if k == 0:
        return 0.0
    product: f64
    product = 1.0
    i: i32
    for i in range(k):
        if x[i] < 1:
            raise ValueError('geometric mean requires a non-empty dataset  containing positive numbers')
        product *= x[i]
    ans: f64
    ans = product**(1/k)
    return ans

@overload
def harmonic_mean(x: list[i32]) -> f64:
    k: i32 = len(x)
    if k == 0:
        return 0.0
    sum: f64
    sum = 0.0
    i: i32
    for i in range(k):
        if x[i] < 0:
            raise ValueError('harmonic mean does not support negative values')
        if x[i] ==0:
            return 0.0
        sum += 1 / x[i]
    ans: f64
    ans = k/sum
    return ans
