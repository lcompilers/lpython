from ltypes import TypeVar, f64, i32, restriction

T = TypeVar('T')

@restriction
def zero(x: T) -> T:
    pass

@restriction
def plus(x: T, y: T) -> T:
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
        res = plus(res, x[i])
    return div(res, k)

print(mean([1,2,3]))
print(mean([1.0,2.0,3.0]))
print(mean(["a","b","c"], zero=empty_string, plus=add_string, div=div_string))