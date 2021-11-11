import python_ast
import ast

input = open("doconcurrentloop_01.py").read()
a = ast.parse(input, type_comments=True)
print(ast.unparse(a))
print()
print(ast.dump(a))

# Transform ast.AST to python_ast.AST:

class Transform(ast.NodeVisitor):

    def generic_visit(self, node):
        d = {}
        for field, value in ast.iter_fields(node):
            if isinstance(value, list):
                new_list = []
                for item in value:
                    if isinstance(item, ast.AST):
                        new_list.append(self.visit(item))
                d[field] = new_list
            elif isinstance(value, ast.AST):
                d[field] = self.visit(value)
            elif isinstance(value, (str, int)):
                d[field] = value
            elif value is None:
                d[field] = value
            else:
                print(type(value))
                raise Exception("Unsupported type")
        class_name = node.__class__.__name__
        #print("V:", class_name)
        new_ast = getattr(python_ast, class_name)
        #print(ast.dump(node))
        #print(d)
        new_node = new_ast(**d)
        return new_node


print()
v = Transform()
a2 = v.visit(a)
print(a2)


# Visit python_ast.AST

v = python_ast.GenericASTVisitor()
v.visit(a2)
