from ltypes import TypeVar, SupportsPlus, SupportsZero, Divisible
from ltypes import f64, i32

T = TypeVar('T', bound=SupportsPlus|SupportsZero|Divisible)

def mean(x: list[T]) -> f64:
    k: i32 = len(x)
    if k == 0:
        return 0.0
    sum: T
    sum = 0
    i: i32
    for i in range(k):
        sum = sum + x[i]
    return sum/k

print(mean([1,2,3]))
print(mean([1.0,2.0,3.0]))