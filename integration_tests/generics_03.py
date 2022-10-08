T = TypeVar('T')

def g(x: T) -> i32:
    return 1

def f(x: T) -> i32:
    return g(x)

print(f(1))