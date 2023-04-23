from lpython import i32
from sys import argv

def test():
    exe: str = argv[0]
    res: str = ""

    if exe[0] == '.':
        exe = exe[1:len(exe)]

    s: str
    for s in exe:
        if s == '.':
            break
        if s == '/':
            res = ""
        else:
            res += s

    assert res == 'test_argv_01'

test()
