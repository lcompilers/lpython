with A() as a, B() as b:
    f(a, b)

with (
    A() as a,
    B(a) as b,
    C(a, b) as c,
):
    f(a, c)

with open('examples/expr2.py', 'r') as file:
    x = file.read()

with open(...) as f: ...

with open('examples/expr2.py', 'r') as file: x = file.read()
