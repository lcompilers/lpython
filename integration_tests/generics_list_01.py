from ltypes import TypeVar
from ltypes import f64, i32

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

def mean(x: list[T]) -> f64:
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
#print(sum(["a","b","c"], zero=empty_string, plus=empty_string))