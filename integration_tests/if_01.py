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

def Verify():
    Test_if_01()

Verify()
