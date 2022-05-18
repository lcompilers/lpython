import sys
import python_ast
import ast

filename = sys.argv[1]
#filename = "doconcurrentloop_01.py"
#filename = "expr2.py"

input = open(filename).read()
a = ast.parse(input, type_comments=True)
#print(ast.unparse(a))
#print()
#print(ast.dump(a))

# Transform ast.AST to python_ast.AST:

def get_newlines(s):
    newlines = []
    for pos in range(len(s)):
        if s[pos] == "\n":
            newlines.append(pos)
    return newlines

newlines = get_newlines(input)

# line and col starts from 1
# It returns a linear position, which starts from 0
def linecol_to_pos(line, col, newlines):
    if line <= 0:
        return 0
    elif line == 1:
        return col - 1
    elif line-1 >= len(newlines):
        return newlines[-1] + 1 + col - 1
    else:
        return newlines[line-2] + 1 + col - 1

class Transform(ast.NodeVisitor):

    # Transform Constant to specific Constant* types
    def visit_Constant(self, node):
        if isinstance(node.value, str):
            new_node = python_ast.ConstantStr(node.value, node.kind)
        elif isinstance(node.value, bool):
            new_node = python_ast.ConstantBool(node.value, node.kind)
        elif isinstance(node.value, int):
            new_node = python_ast.ConstantInt(node.value, node.kind)
        elif isinstance(node.value, float):
            new_node = python_ast.ConstantFloat(node.value, node.kind)
        elif isinstance(node.value, complex):
            new_node = python_ast.ConstantComplex(node.value.real,
                        node.value.imag, node.kind)
        elif isinstance(node.value, Ellipsis.__class__):
            new_node = python_ast.ConstantEllipsis(node.kind)
        elif isinstance(node.value, None.__class__):
            new_node = python_ast.ConstantNone(node.kind)
        else:
            print(type(node.value))
            raise Exception("Unsupported Constant type")
        new_node.first = linecol_to_pos(node.lineno, node.col_offset+1, newlines)
        new_node.last = linecol_to_pos(node.end_lineno, node.end_col_offset, newlines)
        return new_node

    def generic_visit(self, node):
        d = {}
        class_name = node.__class__.__name__
        for field, value in ast.iter_fields(node):
            if field == "ops": # For Compare()
                # We only represent one comparison operator
                assert len(value) == 1
                d[field] = self.visit(value[0])
            elif isinstance(value, list):
                new_list = []
                for item in value:
                    if isinstance(item, ast.AST):
                        new_list.append(self.visit(item))
                d[field] = new_list
            elif field in ["vararg", "kwarg"]:
                if value is None:
                    d[field] = []
                else:
                    d[field] = [self.visit(value)]
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
        if hasattr(node, "lineno"):
            new_node.first = linecol_to_pos(node.lineno, node.col_offset+1, newlines)
            new_node.last = linecol_to_pos(node.end_lineno, node.end_col_offset, newlines)
        else:
            new_node.first = 1
            new_node.last = 1
        return new_node


#print()
v = Transform()
a2 = v.visit(a)
#print(a2)


# Test the visitor python_ast.AST
v = python_ast.GenericASTVisitor()
v.visit(a2)


# Serialize
class Serialization(python_ast.SerializationBaseVisitor):

    def __init__(self):
        # Start with a "mod" class
        self.s = "0 "

    def write_int8(self, i):
        self.s += str(i) + " "

    def write_int64(self, i):
        self.s += str(i) + " "

    def write_float64(self, f):
        self.s += str(f) + " "

    def write_string(self, s):
        self.write_int64(len(s))
        self.s += str(s) + " "

    def write_bool(self, b):
        if b:
            self.write_int8(1)
        else:
            self.write_int8(0)

v = Serialization()
v.visit(a2)
#print()
#print(v.s)

open("ser.txt", "w").write(v.s)
