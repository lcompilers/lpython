from liblfort.ast import parse, dump

source = open("examples/expr2.f90").read()
t = parse(source)
print(source)
print("AST:")
print(dump(t))
