from lpython import i8, i16, i32, i64

def main0():
    x: i8
    y: i16
    z: i32
    w: i64

    x = i8(97)
    y = i16(47)
    z = 56
    w = i64(67)

    print(chr(x), chr(y), chr(z), chr(w))

    assert chr(x) == 'a'
    assert chr(y) == '/'
    assert chr(z) == '8'
    assert chr(w) == 'C'

main0()
