from ltypes import TypeVar

T = TypeVar('T', 'Number')

def sum(lst: list[T], acc: T) -> T:
    i: i32
    for i in range(len(lst)):
        acc = acc + lst[i]
    return acc

print(sum([1,2,3,4,5], 0))