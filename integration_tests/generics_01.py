from ltypes import TypeVar, restriction

T = TypeVar('T')

@restriction
def plus(x: T, y: T) -> T:
    pass

def add_string(x: str, y: str) -> str:
    return x + y

def f(x: T, y: T, **kwargs) -> T:
    return plus(x,y)

print(f(1,2))
print(f("a","b",plus=add_string))
print(f("c","d",plus=add_string))
