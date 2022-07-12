def test_new_line():
    print("abc\n")
    print("\ndef")
    print("x\nyz")

def test_int():
    i: i8 = 1
    j: i16 = 2
    k: i32 = 3
    l: i64 = 4
    print("abc:", i, j, k, l)

test_new_line()
test_int()
