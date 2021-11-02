import ast

input = open("doconcurrentloop_01.py").read()
a = ast.parse(input)
print(ast.unparse(a))
print()
print(ast.dump(a))
