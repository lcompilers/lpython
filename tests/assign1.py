def test_augassign():
    r: i32
    s: i32
    r = 0
    r += 4
    s = 5
    r *= s
    r -= 2
    s = 10
    r /= s
    a: str
    a = ""
    a += "test"

test_augassign()
