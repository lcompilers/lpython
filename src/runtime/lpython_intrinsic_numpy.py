from ltypes import f64, ccall, vectorize

@ccall
def _lfortran_dsin(x: f64) -> f64:
    pass

@vectorize
def sin(x: f64) -> f64:
    return _lfortran_dsin(x)
