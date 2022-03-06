from ltypes import f64, ccall

def random() -> f64:
    return _lfortran_random_float()

@ccall
def _lfortran_random_float() -> f64:
    pass
