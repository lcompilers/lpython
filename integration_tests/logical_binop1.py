from ltypes import i32

def test_issue_1487_1():
    a : i32
    b : i32
    x : i32
    y : i32
    or_op1 : i32
    or_op2 : i32
    a = 1
    b = 2
    x = 0
    y = 4
    or_op1 = a or b
    or_op2 = x or y
    assert or_op1 == 1
    assert or_op2 == 4
    y = 0
    or_op2 = x or y
    assert or_op2 == 0


def test_issue_1487_2():
    a : i32
    b : i32
    x : i32
    y : i32
    and_op1 : i32
    and_op2 : i32
    a = 100
    b = 150
    x = 175
    y = 0
    and_op1 = a and b
    and_op2 = y and x
    assert and_op1 == 150
    assert and_op2 == 0
    x = 0
    and_op2 = y and x
    and_op1 = b and a
    assert and_op2 == 0
    assert and_op1 == 100


def check():
    test_issue_1487_1()
    test_issue_1487_2()


check()
