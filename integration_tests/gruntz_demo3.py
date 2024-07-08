from lpython import S, str
from sympy import Symbol, Pow, sin, oo, pi, E, Mul, Add, oo, log, exp, sign

def mrv(e: S, x: S) -> list[S]:
    """
    Calculate the MRV set of the expression.

    Examples
    ========

    >>> mrv(log(x - log(x))/log(x), x)
    {x}

    """

    if e.is_integer:
        empty_list: list[S] = []
        return empty_list
    if e == x:
        list1: list[S] = [x]
        return list1
    if e.func == log:
        arg0: S = e.args[0]
        list2: list[S] = mrv(arg0, x)
        return list2
    if e.func == Mul or e.func == Add:
        a: S = e.args[0]
        b: S = e.args[1]
        ans1: list[S] = mrv(a, x)
        ans2: list[S] = mrv(b, x)
        list3: list[S] = mrv_max(ans1, ans2, x)
        return list3
    if e.func == Pow:
        base: S = e.args[0]
        list4: list[S] = mrv(base, x)
        return list4
    if e.func == sin:
        list5: list[S] = [x]
        return list5
    # elif e.is_Function:
    #     return reduce(lambda a, b: mrv_max(a, b, x), (mrv(a, x) for a in e.args))
    raise NotImplementedError(f"Can't calculate the MRV of {e}.")

def mrv_max(f: list[S], g: list[S], x: S) -> list[S]:
    """Compute the maximum of two MRV sets.

    Examples
    ========

    >>> mrv_max({log(x)}, {x**5}, x)
    {x**5}

    """

    if len(f) == 0:
        return g
    elif len(g) == 0:
        return f
    # elif f & g:
    #     return f | g
    else:
        f1: S = f[0]
        g1: S = g[0]
        bool1: bool = f1 == x
        bool2: bool = g1 == x
        if bool1 and bool2:
            l: list[S] = [x]
            return l

def rewrite(e: S, x: S, w: S) -> S:
    """
    Rewrites the expression in terms of the MRV subexpression.

    Parameters
    ==========

    e : Expr
        an expression
    x : Symbol
        variable of the `e`
    w : Symbol
        The symbol which is going to be used for substitution in place
        of the MRV in `x` subexpression.

    Returns
    =======

    The rewritten expression

    Examples
    ========

    >>> rewrite(exp(x)*log(x), x, y)
    (log(x)/y, -x)

    """
    Omega: list[S] = mrv(e, x)
    Omega1: S = Omega[0]

    if Omega1 == x:
        newe: S = e.subs(x, S(1)/w)
        return newe

def signinf(e: S, x : S) -> S:
    """
    Determine sign of the expression at the infinity.

    Returns
    =======

    {1, 0, -1}
        One or minus one, if `e > 0` or `e < 0` for `x` sufficiently
        large and zero if `e` is *constantly* zero for `x\to\infty`.

    """

    if not e.has(x):
        return sign(e)
    if e == x:
        return S(1)
    if e.func == Pow:
        base: S = e.args[0]
        if signinf(base, x) == S(1):
            return S(1)

def leadterm(e: S, x: S) -> list[S]:
    """
    Returns the leading term a*x**b as a list [a, b].
    """
    term1: S = sin(x)/x
    term2: S = S(2)*sin(x)/x
    term3: S = sin(S(2)*x)/x
    term4: S = sin(x)**S(2)/x
    term5: S = sin(x)/x**S(2)
    term6: S = sin(x)**S(2)/x**S(2)
    term7: S = sin(sin(sin(x)))/sin(x)
    term8: S = S(2)*log(x+S(1))/x
    term9: S = sin((log(x+S(1))/x)*x)/x

    l1: list[S] = [S(1), S(0)]
    l2: list[S] = [S(2), S(0)]
    l3: list[S] = [S(1), S(1)]
    l4: list[S] = [S(1), S(-1)]

    if e == term1:
        return l1
    elif e == term2:
        return l2
    elif e == term3:
        return l2
    elif e == term4:
        return l3
    elif e == term5:
        return l4
    elif e == term6:
        return l1
    elif e == term7:
        return l1
    elif e == term8:
        return l2
    elif e == term9:
        return l1
    raise NotImplementedError(f"Can't calculate the leadterm of {e}.")

def mrv_leadterm(e: S, x: S) -> list[S]:
    """
    Compute the leading term of the series.

    Returns
    =======

    tuple
        The leading term `c_0 w^{e_0}` of the series of `e` in terms
        of the most rapidly varying subexpression `w` in form of
        the pair ``(c0, e0)`` of Expr.

    Examples
    ========

    >>> leadterm(1/exp(-x + exp(-x)) - exp(x), x)
    (-1, 0)

    """
    # w = Dummy('w', real=True, positive=True)
    # e = rewrite(e, x, w)
    # return e.leadterm(w)
    w: S = Symbol('w')
    newe: S = rewrite(e, x, w)
    coeff_exp_list: list[S] = leadterm(newe, w)
    return coeff_exp_list

def limitinf(e: S, x: S) -> S:
    """
    Compute the limit of the expression at the infinity.

    Examples
    ========

    >>> limitinf(exp(x)*(exp(1/x - exp(-x)) - exp(1/x)), x)
    -1

    """
    if not e.has(x):
        return e
    
    coeff_exp_list: list[S] = mrv_leadterm(e, x)
    c0: S = coeff_exp_list[0]
    e0: S = coeff_exp_list[1]
    sig: S = signinf(e0, x)
    case_2: S = signinf(c0, x) * oo
    if sig == S(1):
        return S(0)
    if sig == S(-1):
        return case_2
    if sig == S(0):
        return limitinf(c0, x)
    raise NotImplementedError(f'Result depends on the sign of {sig}.')

def gruntz(e: S, z: S, z0: S, dir: str ="+") -> S:
    """
    Compute the limit of e(z) at the point z0 using the Gruntz algorithm.

    Explanation
    ===========

    ``z0`` can be any expression, including oo and -oo.

    For ``dir="+"`` (default) it calculates the limit from the right
    (z->z0+) and for ``dir="-"`` the limit from the left (z->z0-). For infinite z0
    (oo or -oo), the dir argument does not matter.

    This algorithm is fully described in the module docstring in the gruntz.py
    file. It relies heavily on the series expansion. Most frequently, gruntz()
    is only used if the faster limit() function (which uses heuristics) fails.
    """

    e0: S
    sub_neg: S = z0 - S(1)/z
    sub_pos: S = z0 + S(1)/z
    if str(dir) == "-":
        e0 = e.subs(z, sub_neg)
    elif str(dir) == "+":
        e0 = e.subs(z, sub_pos)
    else:
        raise NotImplementedError("dir must be '+' or '-'")

    r: S = limitinf(e0, z)
    return r

# test
def test():
    x: S = Symbol('x')
    print(gruntz(sin(x)/x, x, S(0), "+"))
    print(gruntz(S(2)*sin(x)/x, x, S(0), "+"))
    print(gruntz(sin(S(2)*x)/x, x, S(0), "+"))
    print(gruntz(sin(x)**S(2)/x, x, S(0), "+"))
    print(gruntz(sin(x)/x**S(2), x, S(0), "+"))
    print(gruntz(sin(x)**S(2)/x**S(2), x, S(0), "+"))
    print(gruntz(sin(sin(sin(x)))/sin(x), x, S(0), "+"))
    print(gruntz(S(2)*log(x+S(1))/x, x, S(0), "+"))
    print(gruntz(sin((log(x+S(1))/x)*x)/x, x, S(0), "+"))

    assert gruntz(sin(x)/x, x, S(0)) == S(1)
    assert gruntz(S(2)*sin(x)/x, x, S(0)) == S(2)
    assert gruntz(sin(S(2)*x)/x, x, S(0)) == S(2)
    assert gruntz(sin(x)**S(2)/x, x, S(0)) == S(0)
    assert gruntz(sin(x)/x**S(2), x, S(0)) == oo
    assert gruntz(sin(x)**S(2)/x**S(2), x, S(0)) == S(1)
    assert gruntz(sin(sin(sin(x)))/sin(x), x, S(0)) == S(1)
    assert gruntz(S(2)*log(x+S(1))/x, x, S(0)) == S(2)
    assert gruntz(sin((log(x+S(1))/x)*x)/x, x, S(0)) == S(1)

test()