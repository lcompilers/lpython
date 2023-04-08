from lpython import f64, ccall

def time() -> f64:
    return _lfortran_time()

@ccall
def _lfortran_time() -> f64:
    pass
