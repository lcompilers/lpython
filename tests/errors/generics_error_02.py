from ltypes import TypeVar, f64, i32, restriction

T = TypeVar('T')

@restriction
def zero(x: T) -> T:
    pass

@restriction
def add(x: T, y: T) -> T:
    pass

@restriction
def div(x: T, k: i32) -> f64:
    pass

def empty_string(x: str) -> str:
    return ""

def add_string(x: str, y: str) -> str:
    return x + y

def div_string(x: str, k: i32) -> f64:
    return 0.0

def mean(x: list[T], **kwargs) -> f64:
    k: i32 = len(x)
    if k == 0:
        return 0.0
    res: T
    res = zero(x[0])
    i: i32
    for i in range(k):
        res = add(res, x[i])
    return div(res, k)

print(mean(["a","b","c"], add=add_string, div=div_string))