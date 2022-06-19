def test_cast():
    a: i32
    b: f32
    a = 2
    b = 4.2
    a *= b
    b += 1
    a = 5
    a -= 3.9
    a /= b
    b = 3/4
    if a < b:
        print("a < b")

test_cast()
