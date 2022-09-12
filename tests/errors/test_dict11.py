from ltypes import c64, f32

def test_dict():
    d: dict[c64, f32] = {}
    r: c64 = complex(0.1, 0.1)
    d[r] = 1.1
