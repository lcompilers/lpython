from ltypes import TypeVar, Number

T = TypeVar('T', bound=SupportsZero)

def f(x: T) -> T:
    x = 0
    return x

print(f(1))
print(f(1.0))
print(f("a"))
