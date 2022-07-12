T = TypeVar('T')

def f(x: T, y: T) -> T:
    return x + y

print(f(1,2))
print(f("a","b"))
print(f("c","d"))