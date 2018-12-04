from lfortran.ast import ast

class CountNum(ast.GenericASTVisitor):

    def __init__(self):
        self.count = 0

    def visit_Num(self, node):
        self.count += 1


def test_num():
    value = 42
    node = ast.Num(value, lineno=1, col_offset=1)
    assert node.n == value
    v = ast.GenericASTVisitor()
    node.walkabout(v)
    c = CountNum()
    node.walkabout(c)
    assert c.count == 1

def test_expr():
    value = 42
    node = ast.Num(value, lineno=1, col_offset=1)
    p = ast.Print(fmt=None, values=[node], lineno=1, col_offset=1)
    assert p.values[0].n == value
    v = ast.GenericASTVisitor()
    p.walkabout(v)
    c = CountNum()
    p.walkabout(c)
    assert c.count == 1

def test_BinOp():
    i1 = 1
    i2 = 2
    val1 = ast.Num(i1, lineno=1, col_offset=1)
    val2 = ast.Num(i2, lineno=1, col_offset=1)
    node = ast.BinOp(left=val1, right=val2, op=ast.Add(),
            lineno=1, col_offset=1)
    assert isinstance(node.op, ast.Add)
    assert node.left.n == i1
    assert node.right.n == i2
    v = ast.GenericASTVisitor()
    node.walkabout(v)
    c = CountNum()
    node.walkabout(c)
    assert c.count == 2
