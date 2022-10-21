from ltypes import i32, i64, dataclass, union, Union
from enum import Enum

@union
class B(Union):
    x: i32
    y: i64

class C(Enum):
    RED: i32 = 1
    BLUE: i32 = 5
    WHITE: i32 = 6

@dataclass
class A:
    b: B
    c: i32
    cenum: C

def test_ordering():
    bd: B = B()
    bd.x = 1
    ad: A = A(bd, 2, C.BLUE)
    assert ad.b.x == 1
    assert ad.c == 2
    print(ad.cenum.name)
    assert ad.cenum.value == 5

test_ordering()
