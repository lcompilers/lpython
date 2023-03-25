from lpython import f32, f64, i32, i64

def test_sum():
    arr_i32 :list[i32]
    arr_i32 = [6, 12]
    res_i32 :i32 = sum(arr_i32)
    assert res_i32 == 18

    arr_i64 :list[i64] = [i64(600000000000), i64(1200000000000)]
    res_i64 :i64 = sum(arr_i64)
    assert res_i64 == i64(1800000000000)

    eps1 :f32 = f32(1e-12)
    x :f32 = f32(12.5)
    y :f32 = f32(6.5)
    arr_f32 :list[f32]
    arr_f32 = [x, y]
    res_f32 :f32 = sum(arr_f32)
    assert abs(res_f32 - f32(19.0)) < eps1

    eps2: f64 = 1e-12
    arr_f64: list[f64]
    arr_f64 = [12.6, 6.4]
    res_f64 :f64 = sum(arr_f64)
    assert abs(res_f64 - 19.0) < eps2


test_sum()
