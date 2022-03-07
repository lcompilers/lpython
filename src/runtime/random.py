from ltypes import i32, f64, ccall


def random() -> f64:
    """
    Returns a random floating point number in the range [0.0, 1.0)
    """
    return _lfortran_random_float()

@ccall
def _lfortran_random_float() -> f64:
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

def paretovariate(alpha: f64) -> f64:
    """
    Return a random number from a Pareto distribution with parameter `alpha`.
    """
    u: f64
    u = 1.0 - random()
    return u ** (-1.0 / alpha)
