# Nested Function Calls
getattr(x, y)(a, b,)(5, a=a, b=0)
func(a, _op)(s)
test()()

# Subscriptions
a = [1, 2][0]

a, b = ([1, 2] + [0, 0])[:2]

a, b = (c.d() + [1, 0])[:]

{"a": a, "b": b}[val]

if args[i][:1] in ['', '.']:
    pass

x = [[-c % self for c in reversed(T.rep.rep)][:-1]]

[x for x in G if self.ring.is_unit(x[0])][0]

if x not in\
    z: ...

if (x not in\
    z): ...

if x not\
    in\
    z: ...

if (x not in
    z): ...


def imatmul(a, b):
    "Same as a @= b."
    a @= b
    return a
