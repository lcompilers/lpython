from ltypes import TypeVar

T = TypeVar('T')

def f(x: T, y: T) -> T:
    return x + y

<<<<<<< Updated upstream:integration_tests/generics_01.py
print(f(1,2))
print(f("a","b"))
# Does not work yet
#print(f("c","d"))
=======
#print(f(1,2))
#print(f("a","b"))
#print(f("c","d"))
>>>>>>> Stashed changes:examples/typevar1.py
