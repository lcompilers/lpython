from ltypes import i32, f64, i64, dataclass, ccall, union, Union

@dataclass
class A:
    ax: i32
    ay: f64

@dataclass
class B:
    bx: i64
    by: f64

@dataclass
class C:
    cx: i64
    cy: f64
    cz: f64

@ccall
@union
class D(Union):
    a: A
    b: B
    c: C

def test_struct_union():
    d: D = D()

    aobj: A = A(0, 1.0)
    bobj: B = B(int(2), 7.0)
    cobj: C = C(int(5), 13.0, 8.0)

    d.a = aobj
    print(d.a.ax, d.a.ay)
    assert d.a.ax == 0
    assert abs(d.a.ay - 1.0) <= 1e-12

    d.b = bobj
    print(d.b.bx, d.b.by)
    assert d.b.bx == int(2)
    assert abs(d.b.by - 7.0) <= 1e-12

    d.c = cobj
    print(d.c.cx, d.c.cy, d.c.cz)
    assert d.c.cx == 5
    assert abs(d.c.cy - 13.0) <= 1e-12
    assert abs(d.c.cz - 8.0) <= 1e-12

test_struct_union()
