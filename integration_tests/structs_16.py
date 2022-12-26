from ltypes import i32, i64, dataclass, union, Union

@dataclass
class A:
    @union
    class B(Union):
        x: i32
        y: i64
    b: B
    c: i32

def test_ordering():
    bd: A.B = A.B()
    bd.x = 1
    ad: A = A(bd, 2)
    assert ad.b.x == 1
    assert ad.c == 2

test_ordering()
