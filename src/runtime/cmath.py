from ltypes import c64, ccall, f64

pi: f64 = 3.141592653589793238462643383279502884197
e: f64 = 2.718281828459045235360287471352662497757
tau: f64 = 6.283185307179586


@ccall
def _lfortran_zexp(x: c64) -> c64:
    pass

def exp(x: c64) -> c64:
    return _lfortran_zexp(x)


@ccall
def _lfortran_zlog(x: c64) -> c64:
    pass

def log(x: c64) -> c64:
    return _lfortran_zlog(x)


@ccall
def _lfortran_zsqrt(x: c64) -> c64:
    pass

def sqrt(x: c64) -> c64:
    return _lfortran_zsqrt(x)


@ccall
def _lfortran_zacos(x: c64) -> c64:
    pass

def acos(x: c64) -> c64:
    return _lfortran_zacos(x)


@ccall
def _lfortran_zasin(x: c64) -> c64:
    pass

def asin(x: c64) -> c64:
    return _lfortran_zasin(x)


@ccall
def _lfortran_zatan(x: c64) -> c64:
    pass

def atan(x: c64) -> c64:
    return _lfortran_zatan(x)


@ccall
def _lfortran_zcos(x: c64) -> c64:
    pass

def cos(x: c64) -> c64:
    return _lfortran_zcos(x)


@ccall
def _lfortran_zsin(x: c64) -> c64:
    pass

def sin(x: c64) -> c64:
    return _lfortran_zsin(x)


@ccall
def _lfortran_ztan(x: c64) -> c64:
    pass

def tan(x: c64) -> c64:
    return _lfortran_ztan(x)


@ccall
def _lfortran_zacosh(x: c64) -> c64:
    pass

def acosh(x: c64) -> c64:
    return _lfortran_zacosh(x)


@ccall
def _lfortran_zasinh(x: c64) -> c64:
    pass

def asinh(x: c64) -> c64:
    return _lfortran_zasinh(x)


@ccall
def _lfortran_zatanh(x: c64) -> c64:
    pass

def atanh(x: c64) -> c64:
    return _lfortran_zatanh(x)


@ccall
def _lfortran_zcosh(x: c64) -> c64:
    pass

def cosh(x: c64) -> c64:
    return _lfortran_zcosh(x)


@ccall
def _lfortran_zsinh(x: c64) -> c64:
    pass

def sinh(x: c64) -> c64:
    return _lfortran_zsinh(x)


@ccall
def _lfortran_ztanh(x: c64) -> c64:
    pass

def tanh(x: c64) -> c64:
    return _lfortran_ztanh(x)
