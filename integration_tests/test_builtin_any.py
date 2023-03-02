from ltypes import f32, f64, i32, i64

def test_all():
    arr_i32 :list[i32]
    arr_i32 = [6, 3]
    res_i32 :bool = all(arr_i32)
    assert res_i32 == True

    arr_i64 :list[i64] = [i64(000000000000), i64(0000000000000)]
    res_i64 :bool = all(arr_i64)
    assert res_i64 == False

    arr_f32 :list[f32]
    arr_f32 = [f32(12.5), f32(6.5)]
    res_f32 :bool = all(arr_f32)
    assert res_f32 == True

    arr_f64: list[f64]
    arr_f64 = [12.6, 0.0]
    res_f64 :bool = all(arr_f64)
    assert res_f64 == True

    arr_bool: list[bool]
    arr_bool = [False, True]
    res_bool :bool = all(arr_bool)
    assert res_bool == True


test_all()
