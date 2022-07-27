from ltypes import f64, f32, ccall, vectorize, overload

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
