from ltypes import i16

def test_increment():
    i: i16
    for i in range(i16(15), i16(-1), -1):
        print(i)

test_increment()
