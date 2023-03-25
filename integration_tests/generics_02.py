from lpython import TypeVar

T = TypeVar('T')

def swap(x: T, y: T):
    temp: T
    temp = x
    x = y
    y = temp
    print(x)
    print(y)

swap(1,2)