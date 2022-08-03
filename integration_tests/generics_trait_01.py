T = TypeVar('T', 'Number')

def f(a: T, b: T) -> T:
    return a + b

print(f(1,2))