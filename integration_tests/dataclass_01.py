from ltypes import i32, f32
from dataclasses import dataclass

@dataclass
class A:
    i: i32
    f: f32

def main():
    a = A(4, 6.0)
    assert a.i == 4
    assert a.f == 6.0
    a.i = 5
    assert a.i == 5


main()
