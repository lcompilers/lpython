from sizeof_02_module import A
from lpython import i64, sizeof

def get_sizeof() -> i64:
    return sizeof(A)

def test():
    print(get_sizeof())

test()
