from lpython import S
from sympy import Symbol

def mrv(e: S, x: S) -> tuple[dict[S, S], S]:
    if not e.has(x):
        empty_dict : dict[S, S] = {}
        return empty_dict, x
    else:
        raise

def test_mrv():
    x: S = Symbol("x")
    y: S = Symbol("y")
    ans: tuple[dict[S, S], S] = mrv(y, x)

test_mrv()
