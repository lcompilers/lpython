def Test_if_01():
    if True:
        print(1)

    if False:
        print(0)

    if 1 < 0:
        print(0)
    else:
        print(1)

    if 1 > 0:
        print(1)
    else:
        print(0)

    if 1 < 0:
        print(1)
    elif 1 > 0:
        print(1)
    else:
        print(0)

def Test_if_02():
    if True:
        print(1)
        if True:
            print(2)
            if False:
                print(3)
            elif True:
                print(4)
            else:
                print(5)
        else:
            print(6)

    if True:
        print(7)
        if False:
            print(8)
            if False:
                print(9)
            else:
                print(10)
        else:
            if True:
                print(11)
            else:
                print(12)
            print(13)
    print(14)

def Verify():
    Test_if_01()
    Test_if_02()

Verify()
