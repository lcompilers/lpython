from lpython import i32
from sys import exit

def test_for():
    i: i32
    j: i32; k: i32;
    k = 0
    for i in range(0, 10):
        if i == 0:
            j = 0
            continue
        if i > 5:
            k = k + i
            break
        if i == 3:
            print(j, k)
            assert j == 0
            assert k == 0
            quit()
    print(j, k)
    exit(2)

test_for()
