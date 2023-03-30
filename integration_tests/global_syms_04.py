from lpython import i32, i64, f64

a: str = "hi"
b: i32 = -24
c: i64 = i64(151)
d: f64 = -68.512

def print_global_symbols():
    print(a)
    print(b)
    print(c)
    print(d)

def test_global_symbols():
    assert b == -24
    assert c == i64(151)
    assert abs(d - (-68.512)) <= 1e-12

def update_global_symbols():
    global b, c, d
    x: f64 = f64(c) * d
    b = i32(int(x))
    y: f64 = f64(b) / 12.0
    c = i64(int(y))
    z: i64 = i64(b) * c
    d = f64(z)

def test_global_symbols_post_update():
    assert b == -10345
    assert c == i64(-862)
    assert abs(d - 8917390.0) <= 1e-12

print_global_symbols()
test_global_symbols()
update_global_symbols()
print_global_symbols()
test_global_symbols_post_update()
