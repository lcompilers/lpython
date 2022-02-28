from ltypes import i32, f64, ccall


pi: f64 = 3.141592653589793238462643383279502884197
e: f64 = 2.718281828459045235360287471352662497757
tau: f64 = 6.283185307179586


def factorial(x: i32) -> i32:
    """
    Computes the factorial of `x`.
    """

    if x < 0:
        return 0
    result: i32
    result = 1
    i: i32
    for i in range(1, x+1):
        result *= i
    return result


def comb(n: i32, k: i32) -> i32:
    """
    Computes the result of `nCk`, i.e, the number of ways to choose `k`
    items from `n` items without repetition and without order.
    """

    if n < k or n < 0:
        return 0
    return factorial(n)//(factorial(k)*factorial(n-k))


def perm(n: i32, k: i32) -> i32:
    """
    Computes the result of `nPk`, i.e, the number of ways to choose `k` items
    from `n` items without repetition and with order.
    """

    if n < k or n < 0:
        return 0
    return factorial(n)//factorial(n-k)


def isqrt(n: i32) -> i32:
    """
    Computes the integer square root of the nonnegative integer `n`.
    """
    if n < 0:
        raise ValueError('`n` should be nonnegative')
    low: i32
    mid: i32
    high: i32
    low = 0
    high = n+1
    while low + 1 < high:
        mid = (low + high)//2
        if mid*mid <= n:
            low = mid
        else:
            high = mid
    return low


def degrees(x: f64) -> f64:
    """
    Convert angle `x` from radians to degrees.
    """
    return x * 180.0 / pi


def radians(x: f64) -> f64:
    """
    Convert angle `x` from degrees to radians.
    """
    return x * pi / 180.0


def fabs(x: f64) -> f64:
    """
    Return the absolute value of `x`.
    """
    if x < 0.0:
        return -x
    return x


def ldexp(x: f64, i: i32) -> f64:
    return x * (2**i)


def exp(x: f64) -> f64:
    """
    Return `e` raised to the power `x`.
    """
    return e**x


def pow(x: f64, y: f64) -> f64:
    """
    Return `x` raised to the power `y`.
    """
    return x**y


def mod(a: i32, b: i32) -> i32:
    """
    Returns a%b
    """
    return a - (a//b)*b


def gcd(a: i32, b: i32) -> i32:
    """
    Returns greatest common divisor of `a` and `b`
    """
    temp: i32
    if a < 0:
        a = -a
    if b < 0:
        b = -b
    while b != 0:
        a = mod(a, b)
        temp = a
        a = b
        b = temp
    return a


def lcm(a: i32, b: i32) -> i32:
    """
    Returns least common multiple of `a` and `b`
    """
    if a < 0:
        a = -a
    if b < 0:
        b = -b
    if a*b == 0:
        return 0
    return (a*b)//gcd(a, b)


@ccall
def _lfortran_dsin(x: f64) -> f64:
    pass

def sin(x: f64) -> f64:
    return _lfortran_dsin(x)
