from enum import Enum
from ltypes import i32

class NegativeNumbers(Enum):
    a: i32 = -1
    b: i32 = -3
    c: i32 = -5
    d: i32 = 0
    e: i32 = 1
    f: i32 = 3
    g: i32 = 5

class WholeNumbers(Enum):
    h: i32 = 0
    i: i32 = 1
    j: i32 = 3
    k: i32 = 5

def test_negative_numbers():
    assert NegativeNumbers.a.value + NegativeNumbers.b.value + NegativeNumbers.c.value == -9
    assert NegativeNumbers.e.value + NegativeNumbers.f.value + NegativeNumbers.g.value == 9
    assert NegativeNumbers.d.value == 0

def test_whole_numbers():
    assert WholeNumbers.k.value + WholeNumbers.i.value + WholeNumbers.j.value == 9
    assert WholeNumbers.h.value == 0
    print(WholeNumbers.k.name, WholeNumbers.i.name, WholeNumbers.j.name)
    print(WholeNumbers.k.value, WholeNumbers.i.value, WholeNumbers.j.value)
    print(WholeNumbers.h.name)
    print(WholeNumbers.h.value)

test_negative_numbers()
test_whole_numbers()
