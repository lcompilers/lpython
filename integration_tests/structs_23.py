from lpython import dataclass, i32, u64, f64

@dataclass
class A:
    a: i32
    b: i32

@dataclass
class B:
    a: u64
    b: f64

def main0():
    s: A = A(b=-24, a=6)
    print(s.a)
    print(s.b)

    assert s.a == 6
    assert s.b == -24

def main1():
    s: B = B(u64(22), b=3.14)
    print(s.a)
    print(s.b)

    assert s.a == u64(22)
    assert abs(s.b - 3.14) <= 1e-12

main0()
main1()
