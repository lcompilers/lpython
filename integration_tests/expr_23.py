from lpython import f32, i32

def flip_sign_check():
    x: f32
    eps: f32 = f32(1e-5)

    number: i32 = 123
    x = f32(5.5)

    if (number%2 == 1):
        x = -x

    assert abs(x - f32(-5.5)) < eps

    number = 124
    x = f32(5.5)

    if (number%2 == 1):
        x = -x

    assert abs(x - f32(5.5)) < eps

flip_sign_check()
