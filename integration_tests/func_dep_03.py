from ltypes import i32, i64

def casti32(x: i64) -> i32:
    y: i32
    y = x
    return y

def casti64(x: i32) -> i64:
    return int(x)

def addi64(x: i32, y: i64) -> i64:
    return casti64(x) + y

def addi32(x: i32, y: i64) -> i32:
    return x + casti32(y)

def test_add():
    assert addi32(5, int(6)) == 11
    assert addi64(7, int(8)) == 15

test_add()
