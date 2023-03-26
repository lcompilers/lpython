from lpython import c64, ccall, f64, overload, c32

pi: f64 = 3.141592653589793238462643383279502884197
e: f64 = 2.718281828459045235360287471352662497757
tau: f64 = 6.283185307179586


@ccall
def _lfortran_zexp(x: c64) -> c64:
    pass

@ccall
def _lfortran_cexp(x: c32) -> c32:
    pass

@overload
def exp(x: c64) -> c64:
    return _lfortran_zexp(x)

@overload
def exp(x: c32) -> c32:
    return _lfortran_cexp(x)


@ccall
def _lfortran_zlog(x: c64) -> c64:
    pass

@ccall
def _lfortran_clog(x: c32) -> c32:
    pass

@overload
def log(x: c64) -> c64:
    return _lfortran_zlog(x)

@overload
def log(x: c32) -> c32:
    return _lfortran_clog(x)


@ccall
def _lfortran_zsqrt(x: c64) -> c64:
    pass

@ccall
def _lfortran_csqrt(x: c32) -> c32:
    pass

@overload
def sqrt(x: c64) -> c64:
    return _lfortran_zsqrt(x)

@overload
def sqrt(x: c32) -> c32:
    return _lfortran_csqrt(x)


@ccall
def _lfortran_zacos(x: c64) -> c64:
    pass

@ccall
def _lfortran_cacos(x: c32) -> c32:
    pass

@overload
def acos(x: c64) -> c64:
    return _lfortran_zacos(x)

@overload
def acos(x: c32) -> c32:
    return _lfortran_cacos(x)

@ccall
def _lfortran_zasin(x: c64) -> c64:
    pass

@ccall
def _lfortran_casin(x: c32) -> c32:
    pass

@overload
def asin(x: c64) -> c64:
    return _lfortran_zasin(x)

@overload
def asin(x: c32) -> c32:
    return _lfortran_casin(x)

@ccall
def _lfortran_zatan(x: c64) -> c64:
    pass

@ccall
def _lfortran_catan(x: c32) -> c32:
    pass

@overload
def atan(x: c64) -> c64:
    return _lfortran_zatan(x)

@overload
def atan(x: c32) -> c32:
    return _lfortran_catan(x)

@ccall
def _lfortran_zcos(x: c64) -> c64:
    pass

@ccall
def _lfortran_ccos(x: c32) -> c32:
    pass

@overload
def cos(x: c64) -> c64:
    return _lfortran_zcos(x)

@overload
def cos(x: c32) -> c32:
    return _lfortran_ccos(x)

@ccall
def _lfortran_zsin(x: c64) -> c64:
    pass

@ccall
def _lfortran_csin(x: c32) -> c32:
    pass

@overload
def sin(x: c64) -> c64:
    return _lfortran_zsin(x)

@overload
def sin(x: c32) -> c32:
    return _lfortran_csin(x)

@ccall
def _lfortran_ztan(x: c64) -> c64:
    pass

@ccall
def _lfortran_ctan(x: c32) -> c32:
    pass

@overload
def tan(x: c64) -> c64:
    return _lfortran_ztan(x)

@overload
def tan(x: c32) -> c32:
    return _lfortran_ctan(x)


@ccall
def _lfortran_zacosh(x: c64) -> c64:
    pass

@ccall
def _lfortran_cacosh(x: c32) -> c32:
    pass

@overload
def acosh(x: c64) -> c64:
    return _lfortran_zacosh(x)

@overload
def acosh(x: c32) -> c32:
    return _lfortran_cacosh(x)

@ccall
def _lfortran_zasinh(x: c64) -> c64:
    pass

@ccall
def _lfortran_casinh(x: c32) -> c32:
    pass

@overload
def asinh(x: c64) -> c64:
    return _lfortran_zasinh(x)

@overload
def asinh(x: c32) -> c32:
    return _lfortran_casinh(x)

@ccall
def _lfortran_zatanh(x: c64) -> c64:
    pass

@ccall
def _lfortran_catanh(x: c32) -> c32:
    pass

@overload
def atanh(x: c64) -> c64:
    return _lfortran_zatanh(x)

@overload
def atanh(x: c32) -> c32:
    return _lfortran_catanh(x)


@ccall
def _lfortran_zcosh(x: c64) -> c64:
    pass

@ccall
def _lfortran_ccosh(x: c32) -> c32:
    pass

@overload
def cosh(x: c64) -> c64:
    return _lfortran_zcosh(x)

@overload
def cosh(x: c32) -> c32:
    return _lfortran_ccosh(x)

@ccall
def _lfortran_zsinh(x: c64) -> c64:
    pass

@ccall
def _lfortran_csinh(x: c32) -> c32:
    pass

@overload
def sinh(x: c64) -> c64:
    return _lfortran_zsinh(x)

@overload
def sinh(x: c32) -> c32:
    return _lfortran_csinh(x)


@ccall
def _lfortran_ztanh(x: c64) -> c64:
    pass

@ccall
def _lfortran_ctanh(x: c32) -> c32:
    pass

@overload
def tanh(x: c64) -> c64:
    return _lfortran_ztanh(x)

@overload
def tanh(x: c32) -> c32:
    return _lfortran_ctanh(x)
