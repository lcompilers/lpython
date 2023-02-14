from ltypes import i32

def test_issue_1487_1(): # OR operator: a or b
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


def test_issue_1487_2(): # AND operator: a and b
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


def test_XOR(): # XOR (exclusive OR) operator: a ^ b
    a : i32
    b : i32
    x : i32
    y : i32
    xor_op1 : i32
    xor_op2 : i32
    a = 8
    b = 4
    x = 100
    y = 0
    xor_op1 = b ^ a
    xor_op2 = y ^ x
    assert xor_op1 == 12
    assert xor_op2 == 100


def test_NOR(): # NOR operator: not(a or b)
    a : i32
    b : i32
    x : i32
    y : i32
    nor_op1 : bool
    nor_op2 : bool
    a = 0
    b = 0
    x = 1
    y = 0
    nor_op1 = not(b or a)
    nor_op2 = not(y or x)
    assert nor_op1 == True
    assert nor_op2 == False


def test_NAND(): # NAND operator: not(a and b)
    a : i32
    b : i32
    x : i32
    y : i32
    nand_op1 : bool
    nand_op2 : bool
    a = 0
    b = 0
    x = 1
    y = 1
    nand_op1 = not(b and a)
    nand_op2 = not(y and x)
    assert nand_op1 == True
    assert nand_op2 == False


def test_XNOR(): # XNOR operator: ==
    a : i32
    b : i32
    x : i32
    y : i32
    xnor_op1 : bool
    xnor_op2 : bool
    a = 0
    b = 0
    x = 1
    y = 0
    xnor_op1 = b == a
    xnor_op2 = y == x
    assert xnor_op1 == True
    assert xnor_op2 == False


def test_implies(): # implies operator: <=
    a : bool
    b : bool
    x : bool
    y : bool
    imp_op1 : bool
    imp_op2 : bool
    a = True
    b = True
    x = False
    y = True
    imp_op1 = b <= a
    imp_op2 = y <= x
    assert imp_op1 == True
    assert imp_op2 == False


def check():
    test_issue_1487_1()
    test_issue_1487_2()
    test_XOR()
    test_NOR()
    test_NAND()
    test_XNOR()
    test_implies()


check()