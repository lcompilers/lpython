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
