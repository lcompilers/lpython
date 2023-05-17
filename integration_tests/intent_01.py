from lpython import i32, u32, f64, dataclass, In, Out, InOut

@dataclass
class Foo:
    p: i32

def f(x: i32, y: In[f64], z: InOut[list[u32]], w: Out[Foo]):
    assert (x == -12)
    assert abs(y - (4.44)) <= 1e-12
    z.append(u32(5))
    w.p = 24


def main0():
    a: i32 = (-12)
    b: f64 = 4.44
    c: list[u32] = [u32(1), u32(2), u32(3), u32(4)]
    d: Foo = Foo(25)

    print(a, b, c, d.p)

    f(a, b, c, d)
    assert c[-1] == u32(5)
    assert d.p == 24

main0()
