from lfortran.ast import parse, dump, print_tree
from lfortran.ast.tests.test_parser import to_tuple

source = open("examples/expr2.f90").read()
t = parse(source)
print(source)
print()
print("AST (tuple):")
print(to_tuple(t))
print()
print("AST (dump):")
print(dump(t))
print()
print("AST (dump_tree):")
print_tree(t)
