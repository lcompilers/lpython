from lpython import i32, i64, f64

def test():
    a : i32 = 10
    b : f64 = 20.0
    a *= b
    a /= b

    e : i64 = i64(15)
    f : bool = True
    e *= f
    e /= f

test()