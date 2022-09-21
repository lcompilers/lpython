from ltypes import c32, f64

def test_dict():
    d: dict[c32, f64] = {}
    r: c32 = complex(0.1, 0.1)
    d[r] = 1.1
