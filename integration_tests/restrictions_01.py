from ltypes import TypeVar

T = TypeVar('T')

@restriction
def plus(x: T, y: T) -> T:
    pass

def f(x: T, y: T) -> T:
    return plus(x,y)

# print(f(1,2,plus=int.__add__))
print(f(1,2))
'''
print(f("a","b"))
print(f("c","d"))
'''
