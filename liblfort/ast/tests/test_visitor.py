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
