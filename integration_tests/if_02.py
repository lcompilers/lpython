from ltypes import bool, i32

def test_if_01():
    x: bool = True
    y: i32 = 10
    z: i32 = 0

    if x:
        x = False
    if x:
        z += 1
    else:
        z += 1

    if y > 5:
        z += 1
    if y < 12:
        z += 1
    if y == 10:
        z += 1
    if y != 10:
        z += 1
    else:
        z += 1
    if y <= 10:
        z += 1

    y = 5
    if y <= 10:
        z += 1
    if y >= 5:
        z += 1
    if y >= 2:
        z += 1

    # TODO: replace this an assert statement
    print(z)

def verify():
    test_if_01()

verify()
