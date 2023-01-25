from ltypes import TypeVar, restriction

T = TypeVar('T')

@restriction
def add(x: T, y: T) -> T:
    pass

def add_integer(x: i32, y: i32) -> i32:
    return x + y

def add_string(x: str, y: str) -> str:
    return x + y

def f(x: T, y: T, **kwargs) -> T:
    return add(x,y)

print(f(1,2, add=add_integer))
print(f("a","b",add=add_string))
print(f("c","d",add=add_string))
