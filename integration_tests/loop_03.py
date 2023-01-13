from ltypes import i32

def test_loop_01():
    i: i32 = 0
    j: i32 = 0
    rt_inc: i32
    rt_inc = 1

    while False:
        assert False

    while i < 0:
        assert False

    while i < 10:
        i += 1
    assert i == 10

    while i < 20:
        while i < 15:
            i += 1
        i += 1
    assert i == 20

    for i in range(0, 5, rt_inc):
        assert i == j
        j += 1

def test_loop_02():
    i: i32 = 0
    j: i32 = 0
    rt_inc_neg_1: i32 = -1
    rt_inc_1: i32 = 1

    j = 0
    for i in range(10, 0, rt_inc_neg_1):
        j = j + i
    assert j == 55

    for i in range(0, 5, rt_inc_1):
        if i == 3:
            break
    assert i == 3

    j = 0
    for i in range(0, 5, rt_inc_1):
        if i == 3:
            continue
        j += 1
    assert j == 4

def verify():
    test_loop_01()
    test_loop_02()

verify()
