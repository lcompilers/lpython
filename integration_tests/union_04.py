from union_01 import u_type
from lpython import f32, f64, i64

# Test taken from union_01.py but checks importing union

def test_union_04():
    unionobj: u_type = u_type()
    unionobj.integer32 = 1
    print(unionobj.integer32)
    assert unionobj.integer32 == 1

    unionobj.real32 = f32(2.0)
    print(unionobj.real32)
    assert abs(f64(unionobj.real32) - 2.0) <= 1e-6

    unionobj.real64 = 3.5
    print(unionobj.real64)
    assert abs(unionobj.real64 - 3.5) <= 1e-12

    unionobj.integer64 = i64(4)
    print(unionobj.integer64)
    assert unionobj.integer64 == i64(4)

test_union_04()
