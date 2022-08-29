from ltypes import TypeVar

T = TypeVar('T')

@restriction
def plus(x: T, y: T) -> T:
    pass

def f(x: T, y: T) -> T:
    return plus(x, y) 

print(f(1,"a"))