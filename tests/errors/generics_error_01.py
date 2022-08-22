from ltypes import TypeVar, Any

T = TypeVar('T', bound=SupportsPlus)

def f(x: T, y: T) -> T:
    return x + y

print(f(1,"a"))