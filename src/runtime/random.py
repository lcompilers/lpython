from lpython import i32, f64, ccall

e: f64 = 2.718281828459045235360287471352662497757
eps: f64 = 1e-16

#: TODO: Call `log` from C directly until we fix the multiple import issue
def _log(x: f64) -> f64:
    return _lfortran_dlog(x)

@ccall
def _lfortran_dlog(x: f64) -> f64:
    pass

def _exp(x: f64) -> f64:
    return e**x

def _sqrt(x: f64) -> f64:
    return x**(1/2)

def _abs(x: f64) -> f64:
    if x < 0.0:
        return -x
    return x

def random() -> f64:
    """
    Returns a random floating point number in the range [0.0, 1.0)
    """
    return _lfortran_random()

@ccall
def _lfortran_random() -> f64:
    pass

@overload
def seed() -> None:
    """
    Initializes the random number generator.
    """
    _lfortran_init_random_clock()
    return

@overload
def seed(seed: i32) -> None:
    """
    Initializes the random number generator.
    """
    _lfortran_init_random_seed(seed)
    return

@ccall
def _lfortran_init_random_clock() -> None:
    pass

@ccall
def _lfortran_init_random_seed(seed: i32) -> None:
    pass

def randrange(lower: i32, upper: i32) -> i32:
    """
    Return a random integer N such that `lower <= N < upper`.
    """
    return _lfortran_randrange(lower, upper)

@ccall
def _lfortran_randrange(lower: i32, upper: i32) -> i32:
    pass

def randint(lower: i32, upper: i32) -> i32:
    """
    Return a random integer N such that `lower <= N <= upper`.
    """
    return _lfortran_random_int(lower, upper)

@ccall
def _lfortran_random_int(lower: i32, upper: i32) -> i32:
    pass

def uniform(a: f64, b: f64) -> f64:
    """
    Get a random number in the range [a, b) or [a, b] depending on rounding.
    """
    return a + (b - a) * random()

def paretovariate(alpha: f64) -> f64:
    """
    Return a random number from a Pareto distribution with parameter `alpha`.
    """
    u: f64
    u = 1.0 - random()
    return u ** (-1.0 / alpha)

def expovariate(l: f64) -> f64:
    """
    Return a random number from an exponential distribution with parameter
    `l` (lambda).
    """
    assert _abs(l) > eps
    return -_log(1.0 - random()) / l

def weibullvariate(alpha: f64, beta: f64) -> f64:
    """
    Return a random number from a Weibull distribution with parameters `alpha`
    and `beta`.
    """
    assert _abs(beta) > eps
    return alpha * (-_log(1.0 - random())) ** (1.0 / beta)
