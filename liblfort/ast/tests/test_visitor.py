from liblfort import ast

class SubroutinesVisitor(ast.ast.GenericASTVisitor):

    def __init__(self):
        self.subroutine_list = []

    def visit_Subroutine(self, node):
        self.subroutine_list.append("subroutine %s(%s)" % (node.name,
            ", ".join([x.arg for x in node.args])))
        self.visit_sequence(node.body)

    def get_subroutine_list(self):
        return "\n".join(self.subroutine_list)


def test_list_subroutines():
    source = """\
module subroutine1
implicit none

contains

subroutine sub1(a, b)
integer, intent(in) :: a
integer, intent(out) :: b
b = a + 1
end subroutine

subroutine sub2(c)
integer, intent(out) :: c
call sub1(8, c)
end subroutine

end module
"""
    tree = ast.parse(source)
    v = SubroutinesVisitor()
    v.visit(tree)
    assert v.get_subroutine_list() == """\
subroutine sub1(a, b)
subroutine sub2(c)"""

class SubroutinesVisitor2(ast.utils.NodeVisitor):

    def __init__(self):
        self.subroutine_list = []

    def visit_Subroutine(self, node):
        self.subroutine_list.append("subroutine %s(%s)" % (node.name,
            ", ".join([x.arg for x in node.args])))
        # FIXME: visit_sequence() not defined yet
        self.visit_sequence(node.body)

    def get_subroutine_list(self):
        return "\n".join(self.subroutine_list)


def test_list_subroutines2():
    source = """\
module subroutine1
implicit none

contains

subroutine sub1(a, b)
integer, intent(in) :: a
integer, intent(out) :: b
b = a + 1
end subroutine

subroutine sub2(c)
integer, intent(out) :: c
call sub1(8, c)
end subroutine

end module
"""
    tree = ast.parse(source)
    v = SubroutinesVisitor2()
    v.visit(tree)
    assert v.get_subroutine_list() == """\
subroutine sub1(a, b)
subroutine sub2(c)"""




class DoLoopTransformer(ast.utils.NodeTransformer):

    def visit_DoLoop(self, node):
        if node.head:
            cond = ast.ast.Compare(node.head.var, ast.ast.LtE, node.head.end,
                    lineno=1, col_offset=1)
            if node.head.increment:
                step = node.head.increment
            else:
                step = ast.ast.Num(n="1", lineno=1, col_offset=1)
        else:
            cond = ast.ast.Constant(True, lineno=1, col_offset=1)
        #body = self.visit_sequence(node.body)
        body = node.body
        return ast.ast.WhileLoop(cond, node.body, lineno=1, col_offset=1)


def test_doloop_transformer():
    source = """\
subroutine test()
integer :: i, j
j = 0
do i = 1, 10
    j = j + i
end do
if (j /= 55) error stop
end subroutine
"""
    tree = ast.parse(source, False)
    v = DoLoopTransformer()
    print(ast.dump(tree))
    tree = v.visit(tree)
    print(ast.dump(tree))
