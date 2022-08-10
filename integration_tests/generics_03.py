from ltypes import TypeVar, Any

T = TypeVar('T', bound=Any)

def f(x: T, y: T) -> T:
    return x + y
