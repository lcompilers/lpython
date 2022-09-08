from ltypes import TypeVar

T = TypeVar('T')

@restriction
def add(x: T, y: T) -> T:
    pass

def f(x: T, y: T) -> T:
    return add(x, y) 

print(f(1,"a"))