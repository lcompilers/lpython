T = TypeVar('T')

def sum(lst: list[T]) -> T:
    l: i32
    l = len(lst)
    res: T
    i: i32
    for i in range(l):
        res = res + l[i]
    return res

print(sum([1,2,3]))