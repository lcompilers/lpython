from ltypes import i32

def main0():
    i1: i32 = 10
    i2: i32 = 4
    i1 = 3
    i2 = 5
    print(-i1 ^ -i2)
    assert -i1 ^ -i2 == 6


def test_multiple_assign_1():
    a: i32; b: i32; c: i32
    d: f64; e: f32; g: i32
    g = 5
    d = e = g + 1.0
    a = b = c = 10
    assert a == b
    assert b == c
    assert a == 10
    x: f32; y: f64
    x = y = 23.0
    assert abs(x - 23.0) < 1e-6
    assert abs(y - 23.0) < 1e-12
    assert abs(e - 6.0) < 1e-6
    assert abs(d - 6.0) < 1e-12
    i: list[f64]; j: list[f64]; k: list[f64] = []
    g = 0
    for g in range(10):
        k.append(g*2.0 + 5.0)
    i = j = k
    for g in range(10):
        assert abs(i[g] - j[g]) < 1e-12
        assert abs(i[g] - k[g]) < 1e-12
        assert abs(g*2.0 + 5.0 - k[g]) < 1e-12


def test_issue_928():
    a: i32; b: i32; c: tuple[i32, i32]
    a, b = c = 2, 1
    assert a == 2
    assert b == 1
    assert c[0] == a and c[1] == b


test_multiple_assign_1()
test_issue_928()
main0()
