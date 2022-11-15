from ltypes import i32
def test_loop_01():
    i: i32 = 0
    j: i32 = 0

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

    for i in range(5):
        assert i == j
        j += 1

def test_loop_02():
    i: i32 = 0
    j: i32 = 0

    j = 0
    for i in range(10, 0, -1):
        j = j + i
    assert j == 55

    for i in range(5):
        if i == 3:
            break
    assert i == 3

    j = 0
    for i in range(5):
        if i == 3:
            continue
        j += 1
    assert j == 4

def test_loop_03():
    i: i32 = 0
    j: i32 = 0
    k: i32 = 0
    while i < 10:
        j = 0
        while j < 10:
            k += 1
            if i == 0:
                if j == 3:
                    continue
                else:
                    j += 1
            j += 1
        i += 1
    assert k == 95

    i = 0; j = 0; k = 0
    while i < 10:
        j = 0
        while j < 10:
            k += i + j
            if i == 5:
                if j == 4:
                    break
                else:
                    j += 1
            j += 1
        i += 1
    assert k == 826

    i = 0
    if i == 0:
        while i < 10:
            j = 0
            if j == 0:
                while j < 10:
                    k += 1
                    if i == 9:
                        break
                    j += 1
                i += 1
    assert k == 917

def verify():
    test_loop_01()
    test_loop_02()
    test_loop_03()

verify()
