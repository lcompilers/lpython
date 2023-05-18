from lpython import TypeVar, InOut

T = TypeVar('T')

def swap(x: InOut[T], y: InOut[T]):
    temp: T
    temp = x
    x = y
    y = temp
    print(x)
    print(y)

swap(1,2)
