import python_ast
import ast

input = open("doconcurrentloop_01.py").read()
a = ast.parse(input, type_comments=True)
print(ast.unparse(a))
print()
print(ast.dump(a))

class Transform(ast.NodeVisitor):

    def generic_visit(self, node):
        d = {}
        for field, value in ast.iter_fields(node):
            print(field, value)
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
        print("V:", class_name)
        new_ast = getattr(python_ast, class_name)
        print(ast.dump(node))
        print(d)
        new_node = new_ast(**d)
        return new_node


#v = GenericASTVisitor()
#v.visit(a)

print()
v = Transform()
v.visit(a)
