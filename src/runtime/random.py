from ltypes import i32, f64, ccall

def random() -> f64:
    return _lfortran_random_float()

@ccall
def _lfortran_random_float() -> f64:
    pass

def randint(lower: i32, upper: i32) -> i32:
    return _lfortran_random_int(lower, upper)

@ccall
def _lfortran_random_int(lower: i32, upper: i32) -> i32:
    pass
