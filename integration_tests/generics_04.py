from ltypes import TypeVar, SupportsZero, SupportsPlus

T = TypeVar('T', bound=SupportsZero|SupportsPlus)

def f(x: T) -> T:
    x = 0
    return x

print(f(1))
print(f(1.0))
print(f("a"))
