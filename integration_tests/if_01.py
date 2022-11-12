def Test_if_01():
    z: i32 = 0
    if True:
        assert True

    if False:
        assert False

    if 1 < 0:
        assert False
    else:
        assert True

    if 1 > 0:
        assert True
    else:
        assert False

    if 1 < 0:
        assert False
    elif 1 > 0:
        assert True
    else:
        assert False

def Test_if_02():
    if True:
        print(1)
        if True:
            print(2)
            if False:
                assert False
            elif True:
                print(4)
            else:
                assert False
        else:
            assert False

    if True:
        print(7)
        if False:
            assert False
            if False:
                print(9)
            else:
                print(10)
        else:
            if True:
                print(11)
            else:
                assert False
            print(13)
    print(14)

def Verify():
    Test_if_01()
    Test_if_02()

Verify()
