from ltypes import TypeVar

# 'Number' is not supported by CPython
T = TypeVar('T', 'Number')

def f(x: T, y: T) -> T:
    return x + y

print(f(1,2))
# print(f("a","b"))
# print(f("c","d"))
