from ltypes import TypeVar, restriction, i32, f64

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
    return -1.0

def sum(x: list[T], **kwargs) -> T:
    k: i32 = len(x)
    res: T
    res = zero(x[0])
    i: i32
    for i in range(k):
        res = add(res, x[i])
    return res

def mean(x: list[T], **kwargs) -> f64:
    s: T = sum(x)
    k: i32 = len(x)
    if k == 0:
        return 0.0
    return div(s, k)

print(mean([3,4,5]))
print(mean([1.0,2.0,3.0]))
print(mean(["a","b","c"], zero=empty_string, add=add_string, div=div_string))