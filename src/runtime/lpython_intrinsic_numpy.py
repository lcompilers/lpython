from ltypes import f64, f32, ccall, vectorize, overload
from numpy import empty, int64

n: i32
n = TypeVar("n")

def zeros(n: i32) -> f64[n]:
    A: f64[n]
    A = empty(n)
    i: i32
    for i in range(n):
        A[i] = 0.0
    return A

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
