from liblfort import ast

class SubroutinesVisitor(ast.ast.GenericASTVisitor):

    def visit_Subroutine(self, node):
        print("subroutine %s(%s)" % (node.name, node.args))
        self.visit_sequence(node.body)

def main():
    filename = "examples/random.f90"
    tree = ast.parse_file(filename)
    v = SubroutinesVisitor()
    #v.visit(tree)

if __name__ == "__main__":
    main()
