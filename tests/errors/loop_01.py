from lpython import i16

def test_increment():
    i: i16
    for i in range(15, i16(-1), i16(-1)):
        print(i)

test_increment()
