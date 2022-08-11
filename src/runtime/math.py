from ltypes import i8, i16, i32, f32, i64, f64, ccall, overload


pi: f64 = 3.141592653589793238462643383279502884197
e: f64 = 2.718281828459045235360287471352662497757
tau: f64 = 6.283185307179586

# TODO: Change floor used inside functions implemented here to
# floordiv operator (//) once the multiple import issue is fixed

@overload
def modf(x: f64) -> tuple[f64, f64]:
    """
    Return fractional and integral parts of `x` as a pair.

    Both results carry the sign of x and are floats.
    """
    return (x - int(x), float(int(x)))

@overload
def factorial(x: i32) -> i32:
    """
    Computes the factorial of `x`.
    """

    result: i32
    result = 0
    if x < 0:
        return result
    result = 1
    i: i32
    for i in range(1, x+1):
        result *= i
    return result

@overload
def factorial(x: i64) -> i64:
    """
    Computes the factorial of `x`.
    """
    result: i64
    result = 0
    if x < 0:
        return result
    result = 1
    i: i64
    for i in range(1, x+1):
        result *= i
    return result

@overload
def floor(x: i32) -> i32:
    return x

@overload
def floor(x: i64) -> i64:
    return x

@overload
def floor(x: f64) -> i64:
    r: i64
    r = int(x)
    if x >= 0 or x == r:
        return r
    return r - 1

@overload
def floor(x: f32) -> i32:
    r: i32
    r = int(x)
    if x >= 0 or x == r:
        return r
    return r - 1

@overload
def ceil(x: i32) -> i32:
    return x

@overload
def ceil(x: i64) -> i64:
    return x

@overload
def ceil(x: f64) -> i64:
    r: i64
    r = int(x)
    if x <= 0 or r == x:
        return r
    return r + 1

@overload
def ceil(x: f32) -> i32:
    r: i32
    r = int(x)
    if x <= 0 or r == x:
        return r
    return r + 1

# fsum
# supported data types: i32, i64, f32, f64

@overload
def fsum(arr: list[i32]) -> f64:
    """
    Sum of the elements of `arr`.
    """
    sum: f64
    sum = 0.0

    i: i32
    for i in range(len(arr)):
        sum += arr[i]
    return sum

@overload
def fsum(arr: list[i64]) -> f64:
    """
    Sum of the elements of `arr`.
    """
    sum: f64
    sum = 0.0

    i: i32
    for i in range(len(arr)):
        sum += arr[i]
    return sum

@overload
def fsum(arr: list[f32]) -> f64:
    """
    Sum of the elements of `arr`.
    """
    sum: f64
    sum = 0.0

    i: i32
    for i in range(len(arr)):
        sum += arr[i]
    return sum

@overload
def fsum(arr: list[f64]) -> f64:
    """
    Sum of the elements of `arr`.
    """
    sum: f64
    sum = 0.0

    i: i32
    for i in range(len(arr)):
        sum += arr[i]
    return sum

# prod
# supported data types: i32, i64, f32, f64

@overload
def prod(arr: list[i32]) -> f64:
    """
    Return the product of the elements of `arr`.
    """

    result: f64
    result = 1.0
    i: i32
    for i in range(len(arr)):
        result *= arr[i]
    return result

@overload
def prod(arr: list[i64]) -> f64:
    """
    Return the product of the elements of `arr`.
    """

    result: f64
    result = 1.0
    i: i32
    for i in range(len(arr)):
        result *= arr[i]
    return result

@overload
def prod(arr: list[f32]) -> f64:
    """
    Return the product of the elements of `arr`.
    """

    result: f64
    result = 1.0
    i: i32
    for i in range(len(arr)):
        result *= arr[i]
    return result

@overload
def prod(arr: list[f64]) -> f64:
    """
    Return the product of the elements of `arr`.
    """

    result: f64
    result = 1.0
    i: i32
    for i in range(len(arr)):
        result *= arr[i]
    return result


def comb(n: i32, k: i32) -> i32:
    """
    Computes the result of `nCk`, i.e, the number of ways to choose `k`
    items from `n` items without repetition and without order.
    """

    if n < k or n < 0:
        return 0
    res: i32
    res = floor(factorial(n)/(factorial(k)*factorial(n-k)))
    return res


def perm(n: i32, k: i32) -> i32:
    """
    Computes the result of `nPk`, i.e, the number of ways to choose `k` items
    from `n` items without repetition and with order.
    """

    if n < k or n < 0:
        return 0
    res: i32
    res = floor(factorial(n)/factorial(n-k))
    return res


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
        mid = floor((low + high)/2)
        if mid*mid <= n:
            low = mid
        else:
            high = mid
    return low

# degrees
# supported data types: i8, i16, i32, i64, f32, f64

@overload
def degrees(x: i8) -> f64:
    """
    Convert angle `x` from radians to degrees.
    """
    return x * 180.0 / pi

@overload
def degrees(x: i16) -> f64:
    """
    Convert angle `x` from radians to degrees.
    """
    return x * 180.0 / pi

@overload
def degrees(x: i32) -> f64:
    """
    Convert angle `x` from radians to degrees.
    """
    return x * 180.0 / pi

@overload
def degrees(x: i64) -> f64:
    """
    Convert angle `x` from radians to degrees.
    """
    return x * 180.0 / pi

@overload
def degrees(x: f32) -> f64:
    """
    Convert angle `x` from radians to degrees.
    """
    return x * 180.0 / pi

@overload
def degrees(x: f64) -> f64:
    """
    Convert angle `x` from radians to degrees.
    """
    return x * 180.0 / pi

# radians
# supported data types: i8, i16, i32, i64, f32, f64

@overload
def radians(x: i8) -> f64:
    """
    Convert angle `x` from degrees to radians.
    """
    return x * pi / 180.0

@overload
def radians(x: i16) -> f64:
    """
    Convert angle `x` from degrees to radians.
    """
    return x * pi / 180.0

@overload
def radians(x: i32) -> f64:
    """
    Convert angle `x` from degrees to radians.
    """
    return x * pi / 180.0

@overload
def radians(x: i64) -> f64:
    """
    Convert angle `x` from degrees to radians.
    """
    return x * pi / 180.0

@overload
def radians(x: f32) -> f64:
    """
    Convert angle `x` from degrees to radians.
    """
    return x * pi / 180.0

@overload
def radians(x: f64) -> f64:
    """
    Convert angle `x` from degrees to radians.
    """
    return x * pi / 180.0

# fabs
# supported data types: i32, i64, f32, f64
@overload
def fabs(x: f32) -> f32:
    """
    Return the absolute value of `x`.
    """
    if x < 0.0:
        return -x
    return x

@overload
def fabs(x: f64) -> f64:
    """
    Return the absolute value of `x`.
    """
    if x < 0.0:
        return -x
    return x

@overload
def fabs(x: i64) -> f64:
    """
    Return the absolute value of `x`.
    """
    if x < 0.0:
        return -float(x)
    return float(x)

@overload
def fabs(x: i32) -> f64:
    """
    Return the absolute value of `x`.
    """
    if x < 0.0:
        return -float(x)
    return float(x)

@overload
def fabs(x: i16) -> f64:
    """
    Return the absolute value of `x`.
    """
    if x < 0.0:
        return -float(x)
    return float(x)

@overload
def fabs(x: i8) -> f64:
    """
    Return the absolute value of `x`.
    """
    if x < 0.0:
        return -float(x)
    return float(x)

# pow
# supported data types: i32, i64, f32, f64
@overload
def pow(x: f64, y: f64) -> f64:
    """
    Return `x` raised to the power  `y`.
    """
    if y < 0:
        raise ValueError('y should be nonnegative')
    result: f64
    result = x**y
    return result


@overload
def pow(x: i64, y: i64) -> i64:
    """
    Return `x` raised to the power `y`.
    """
    if y < 0:
        raise ValueError('y should be nonnegative')
    result: i64
    result = x**y
    return result

@overload
def pow(x: f32, y: f32) -> f64:
    """
    Return `x` raised to the power `y`.
    """
    if y < 0:
        raise ValueError('y should be nonnegative')
    result: f64
    result = x**y
    return result

@overload
def pow(x: i32, y: i32) -> i32:
    """
    Return `x` raised to the power `y`.
    """
    if y < 0:
        raise ValueError('y should be nonnegative')
    result: i32
    result = x**y
    return result

@overload
def ldexp(x: f64, i: i32) -> f64:
    result: f64
    result = x * (2**i)
    return result


def exp(x: f64) -> f64:
    """
    Return `e` raised to the power `x`.
    """
    return e**x


def mod(a: i32, b: i32) -> i32:
    """
    Returns a%b
    """
    r: i32
    r = floor(a/b)
    return a - r*b


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
    res: i32
    res = floor((a*b)/gcd(a, b))
    return res


def copysign(x: f64, y: f64) -> f64:
    """
    Return `x` with the sign of `y`.
    """
    if y > 0.0 or (y == 0.0 and atan2(y, -1.0) > 0.0):
        return fabs(x)
    else:
        return -fabs(x)


def hypot(x: i32, y: i32) -> f64:
    """
    Returns the hypotenuse of the right triangle with sides `x` and `y`.
    """
    return sqrt(1.0*(x**2 + y**2))

@overload
def trunc(x: f64) -> i64:
    """
    Return x with the fractional part removed, leaving the integer part.
    """
    if x>0:
        return floor(x)
    else:
        return ceil(x)

@overload
def trunc(x: f32) -> i32:
    """
    Return x with the fractional part removed, leaving the integer part.
    """
    if x>0:
        return floor(x)
    else:
        return ceil(x)

def sqrt(x: f64) -> f64:
    return x**(1/2)

@ccall
def _lfortran_dsin(x: f64) -> f64:
    pass

def sin(x: f64) -> f64:
    return _lfortran_dsin(x)

@ccall
def _lfortran_dcos(x: f64) -> f64:
    pass

def cos(x: f64) -> f64:
    return _lfortran_dcos(x)

@ccall
def _lfortran_dtan(x: f64) -> f64:
    pass

def tan(x: f64) -> f64:
    return _lfortran_dtan(x)

@ccall
def _lfortran_dlog(x: f64) -> f64:
    pass

def log(x: f64) -> f64:
    return _lfortran_dlog(x)

@ccall
def _lfortran_dlog10(x: f64) -> f64:
    pass

def log10(x: f64) -> f64:
    return _lfortran_dlog10(x)

def log2(x: f64) -> f64:
    return _lfortran_dlog(x)/_lfortran_dlog(2.0)

@ccall
def _lfortran_derf(x: f64) -> f64:
    pass

def erf(x: f64) -> f64:
    return _lfortran_derf(x)

@ccall
def _lfortran_derfc(x: f64) -> f64:
    pass

def erfc(x: f64) -> f64:
    return _lfortran_derfc(x)

@ccall
def _lfortran_dgamma(x: f64) -> f64:
    pass

def gamma(x: f64) -> f64:
    return _lfortran_dgamma(x)

@ccall
def _lfortran_dlog_gamma(x: f64) -> f64:
    pass

def lgamma(x: f64) -> f64:
    return _lfortran_dlog_gamma(x)

@ccall
def _lfortran_dasin(x: f64) -> f64:
    pass

def asin(x: f64) -> f64:
    return _lfortran_dasin(x)

@ccall
def _lfortran_dacos(x: f64) -> f64:
    pass

def acos(x: f64) -> f64:
    return _lfortran_dacos(x)

@ccall
def _lfortran_datan(x: f64) -> f64:
    pass

def atan(x: f64) -> f64:
    return _lfortran_datan(x)

@ccall
def _lfortran_datan2(y: f64, x: f64) -> f64:
    pass

def atan2(y: f64, x: f64) -> f64:
    return _lfortran_datan2(y, x)

@ccall
def _lfortran_dsinh(x: f64) -> f64:
    pass

def sinh(x: f64) -> f64:
    return _lfortran_dsinh(x)

@ccall
def _lfortran_dcosh(x: f64) -> f64:
    pass

def cosh(x: f64) -> f64:
    return _lfortran_dcosh(x)

@ccall
def _lfortran_dtanh(x: f64) -> f64:
    pass

def tanh(x: f64) -> f64:
    return _lfortran_dtanh(x)

@ccall
def _lfortran_dasinh(x: f64) -> f64:
    pass

def asinh(x: f64) -> f64:
    return _lfortran_dasinh(x)

@ccall
def _lfortran_dacosh(x: f64) -> f64:
    pass

def acosh(x: f64) -> f64:
    return _lfortran_dacosh(x)

@ccall
def _lfortran_datanh(x: f64) -> f64:
    pass

def atanh(x: f64) -> f64:
    return _lfortran_datanh(x)


def expm1(x: f64) -> f64:
    return exp(x) - 1


def log1p(x: f64) -> f64:
    return log(1 + x)


def fmod(x: f64, y: f64) -> f64:
    if y == 0:
        raise ValueError('math domain error')
    return _lfortran_dfmod(x, y)


@ccall
def _lfortran_dfmod(x: f64, y: f64) -> f64:
    pass


def remainder(x: f64, y: f64) -> f64:
    q: i64
    q = int(x/y)
    if x - y*q > y*(q + 1) - x:
        return x - y*(q + 1)
    return x - y*q
