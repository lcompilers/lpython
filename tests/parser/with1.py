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

with a, b, c as y, \
            z:
    pass

with tag('x'):
    pass

with t as x, y(): ...

with a as b, \
        c(), \
        d(), \
        e as f, \
        g as x:
    pass

# TODO
# with (a as b, c()): ...
# with (a as b, c(),): ...
