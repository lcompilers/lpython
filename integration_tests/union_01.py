from ltypes import Union, i32, i64, f64, f32, ccall, union

@ccall
@union
class u_type(Union):
    integer32: i32
    real32: f32
    real64: f64
    integer64: i64

def test_union():
    unionobj: u_type = u_type()
    unionobj.integer32 = 1
    print(unionobj.integer32)
    assert unionobj.integer32 == 1

    unionobj.real32 = 2.0
    print(unionobj.real32)
    assert abs(unionobj.real32 - 2.0) <= 1e-6

    unionobj.real64 = 3.5
    print(unionobj.real64)
    assert abs(unionobj.real64 - 3.5) <= 1e-12

    unionobj.integer64 = 4
    print(unionobj.integer64)
    assert unionobj.integer64 == 4

test_union()
