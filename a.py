import python_ast
import ast

input = open("doconcurrentloop_01.py").read()
a = ast.parse(input, type_comments=True)
print(ast.unparse(a))
print()
print(ast.dump(a))

# Transform ast.AST to python_ast.AST:

class Transform(ast.NodeVisitor):

    # Transform Constant to specific Constant* types
    def visit_Constant(self, node):
        if isinstance(node.value, str):
            return python_ast.ConstantStr(node.value, node.kind)
        elif isinstance(node.value, int):
            return python_ast.ConstantInt(node.value, node.kind)
        elif isinstance(node.value, float):
            return python_ast.ConstantFloat(node.value, node.kind)
        elif isinstance(node.value, bool):
            return python_ast.ConstantBool(node.value, node.kind)
        else:
            print(type(node.value))
            raise Exception("Unsupported Constant type")

    def generic_visit(self, node):
        d = {}
        class_name = node.__class__.__name__
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
                print("Node type:", class_name)
                print("Value type:", type(value))
                raise Exception("Unsupported value type")
        new_ast = getattr(python_ast, class_name)
        new_node = new_ast(**d)
        return new_node


print()
v = Transform()
a2 = v.visit(a)
print(a2)


# Visit python_ast.AST

v = python_ast.GenericASTVisitor()
v.visit(a2)
