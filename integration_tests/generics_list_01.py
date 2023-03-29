from lpython import TypeVar, f64, i32, restriction

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

def empty_integer(x: i32) -> i32:
    return 0

def add_integer(x: i32, y: i32) -> i32:
    return x + y

def div_integer(x: i32, k: i32) -> f64:
    return x / k

def empty_float(x: f64) -> f64:
    return 0.0

def add_float(x: f64, y: f64) -> f64:
    return x + y

def div_float(x: f64, k: i32) -> f64:
    return x / k

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

print(mean([1,2,3], zero=empty_integer, add=add_integer, div=div_integer))
print(mean([1.0,2.0,3.0], zero=empty_float, add=add_float, div=div_float))
print(mean(["a","b","c"], zero=empty_string, add=add_string, div=div_string))