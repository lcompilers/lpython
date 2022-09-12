from ltypes import f32, f64

def test_dict():
    d: dict[f32, f64] = {}
    r: f32 = 0.1
    d[r] = 1.1
