from lpython import i32, i64, u16, u64, f32, f64

def f():

    i: i32
    j: i64
    i = -67
    j = i64(0)

    print(i, j)
    print(not i, not j)
    assert (not i) == False
    assert (not j) == True

    k: u16
    l: u64
    k = u16(0)
    l = u64(55)

    print(k, l)
    print(not k, not l)
    assert (not k) == True
    assert (not l) == False

    m: f32
    n: f64
    m = f32(0.0)
    n = -3.14

    print(m, n)
    print(not m, not n)
    assert (not m) == True
    assert (not n) == False

f()
