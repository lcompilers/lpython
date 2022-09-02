from ltypes import f64, f32, ccall, vectorize, overload

pi_64: f64 = 3.141592653589793238462643383279502884197
pi_32: f32 = 3.141592653589793238462643383279502884197

########## sin ##########

@ccall
def _lfortran_dsin(x: f64) -> f64:
    pass

@overload
@vectorize
def sin(x: f64) -> f64:
    return _lfortran_dsin(x)

@ccall
def _lfortran_ssin(x: f32) -> f32:
    pass

@overload
@vectorize
def sin(x: f32) -> f32:
    return _lfortran_ssin(x)

########## cos ##########

@ccall
def _lfortran_dcos(x: f64) -> f64:
    pass

@overload
@vectorize
def cos(x: f64) -> f64:
    return _lfortran_dcos(x)

@ccall
def _lfortran_scos(x: f32) -> f32:
    pass

@overload
@vectorize
def cos(x: f32) -> f32:
    return _lfortran_scos(x)

########## sqrt ##########

@overload
@vectorize
def sqrt(x: f64) -> f64:
    return x**(1/2)

@overload
@vectorize
def sqrt(x: f32) -> f32:
    result: f32
    result = x**(1/2)
    return result

########## tan ##########

@ccall
def _lfortran_dtan(x: f64) -> f64:
    pass

@overload
@vectorize
def tan(x: f64) -> f64:
    return _lfortran_dtan(x)

@ccall
def _lfortran_stan(x: f32) -> f32:
    pass

@overload
@vectorize
def tan(x: f32) -> f32:
    return _lfortran_stan(x)


########## sinh ##########

@ccall
def _lfortran_dsinh(x: f64) -> f64:
    pass

@overload
@vectorize
def sinh(x: f64) -> f64:
    return _lfortran_dsinh(x)

@ccall
def _lfortran_ssinh(x: f32) -> f32:
    pass

@overload
@vectorize
def sinh(x: f32) -> f32:
    return _lfortran_ssinh(x)

########## cosh ##########

@ccall
def _lfortran_dcosh(x: f64) -> f64:
    pass

@overload
@vectorize
def cosh(x: f64) -> f64:
    return _lfortran_dcosh(x)

@ccall
def _lfortran_scosh(x: f32) -> f32:
    pass

@overload
@vectorize
def cosh(x: f32) -> f32:
    return _lfortran_scosh(x)


########## log ##########

@ccall
def _lfortran_dlog(x: f64) -> f64:
    pass

@overload
@vectorize
def log(x: f64) -> f64:
    return _lfortran_dlog(x)

@ccall
def _lfortran_slog(x: f32) -> f32:
    pass

@overload
@vectorize
def log(x: f32) -> f32:
    return _lfortran_slog(x)

########## log10 ##########

@ccall
def _lfortran_dlog10(x: f64) -> f64:
    pass

@overload
@vectorize
def log10(x: f64) -> f64:
    return _lfortran_dlog10(x)

@ccall
def _lfortran_slog10(x: f32) -> f32:
    pass

@overload
@vectorize
def log10(x: f32) -> f32:
    return _lfortran_slog10(x)

########## log2 ##########

@overload
@vectorize
def log2(x: f64) -> f64:
    return _lfortran_dlog(x)/_lfortran_dlog(2.0)

@overload
@vectorize
def log2(x: f32) -> f32:
    return _lfortran_slog(x)/_lfortran_slog(2.0)

########## arcsin ##########

@ccall
def _lfortran_dasin(x: f64) -> f64:
    pass

@overload
@vectorize
def arcsin(x: f64) -> f64:
    return _lfortran_dasin(x)

@ccall
def _lfortran_sasin(x: f32) -> f32:
    pass

@overload
@vectorize
def arcsin(x: f32) -> f32:
    return _lfortran_sasin(x)

########## arccos ##########

@ccall
def _lfortran_dacos(x: f64) -> f64:
    pass

@overload
@vectorize
def arccos(x: f64) -> f64:
    return _lfortran_dacos(x)

@ccall
def _lfortran_sacos(x: f32) -> f32:
    pass

@overload
@vectorize
def arccos(x: f32) -> f32:
    return _lfortran_sacos(x)

########## tanh ##########

@ccall
def _lfortran_dtanh(x: f64) -> f64:
    pass

@overload
@vectorize
def tanh(x: f64) -> f64:
    return _lfortran_dtanh(x)

@ccall
def _lfortran_stanh(x: f32) -> f32:
    pass

@overload
@vectorize
def tanh(x: f32) -> f32:
    return _lfortran_stanh(x)

########## exp ##########

@ccall
def _lfortran_dexp(x: f64) -> f64:
    pass

@overload
@vectorize
def exp(x: f64) -> f64:
    return _lfortran_dexp(x)

@ccall
def _lfortran_sexp(x: f32) -> f32:
    pass

@overload
@vectorize
def exp(x: f32) -> f32:
    return _lfortran_sexp(x)

########## arctan ##########

@ccall
def _lfortran_datan(x: f64) -> f64:
    pass

@overload
@vectorize
def arctan(x: f64) -> f64:
    return _lfortran_datan(x)

@ccall
def _lfortran_satan(x: f32) -> f32:
    pass

@overload
@vectorize
def arctan(x: f32) -> f32:
    return _lfortran_satan(x)

########## degrees ##########

@overload
@vectorize
def degrees(x: f64) -> f64:
    return x*180/pi_64

@overload
@vectorize
def degrees(x: f32) -> f32:
    return x*180/pi_32

########## radians ##########

@overload
@vectorize
def radians(x: f64) -> f64:
    return x*pi_64/180

@overload
@vectorize
def radians(x: f32) -> f32:
    return x*pi_32/180
